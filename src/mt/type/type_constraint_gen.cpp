#include "mt/display.hpp"
#include "type_constraint_gen.hpp"
#include "debug.hpp"
#include "type_representation.hpp"
#include "../string.hpp"

#define MT_SCHEME_FUNC_REF (1)
#define MT_SCHEME_FUNC_CALL (1)

namespace mt {

void TypeConstraintGenerator::show_type_distribution() const {
  auto counts = type_store.type_distribution();
  const auto num_types = double(type_store.size());

  for (const auto& ct : counts) {
    std::cout << to_string(ct.first) << ": " << ct.second << " (";
    std::cout << ct.second/num_types << ")" << std::endl;
  }
}

void TypeConstraintGenerator::root_block(const RootBlock& block) {
  ScopeState<const MatlabScope>::Helper matlab_scope_helper(scopes, block.scope);
  ScopeState<const TypeScope>::Helper type_scope_helper(type_scopes, block.type_scope);
  block.block->accept_const(*this);
}

void TypeConstraintGenerator::block(const Block& block) {
  for (const auto& node : block.nodes) {
    node->accept_const(*this);
  }
}

void TypeConstraintGenerator::show_local_function_types(const TypeToString& printer) const {
  for (const auto& func_it : function_type_handles) {
    const auto& def_handle = func_it.first;
    const auto& maybe_type = substitution.bound_type(make_term(nullptr, func_it.second));
    const auto& type = maybe_type ? maybe_type.value() : func_it.second;

    std::string name;

    store.use<Store::ReadConst>([&](const auto& reader) {
      const auto& def = reader.at(def_handle);
      name = string_registry.at(def.header.name.full_name());
    });

    std::cout << printer.color(style::bold) << name << printer.dflt_color() <<
      ":\n  " << printer.apply(type) << std::endl;
  }
}

void TypeConstraintGenerator::show_variable_types(const TypeToString& printer) const {
  for (const auto& var_it : variable_type_handles) {
    const auto& def_handle = var_it.first;
    const auto& maybe_type = substitution.bound_type(make_term(nullptr, var_it.second));
    const auto& type = maybe_type ? maybe_type.value() : var_it.second;

    std::string name;

    store.use<Store::ReadConst>([&](const auto& reader) {
      const auto& def = reader.at(def_handle);
      name = string_registry.at(def.name.full_name());
    });

    std::cout << name << ": " << printer.apply(type) << std::endl;
  }
}

void TypeConstraintGenerator::gather_function_inputs(const MatlabScope& scope,
                                                     const FunctionInputParameters& inputs,
                                                     TypePtrs& into) {
  for (const auto& input : inputs) {
    if (!input.is_ignored) {
      assert(input.name.full_name() >= 0);

      const auto variable_def_handle = scope.local_variables.at(input.name);
      const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

      into.push_back(variable_type_handle);
    } else {
      //  @TODO: Change this type to Top
      into.push_back(make_fresh_type_variable_reference());
    }
  }
}

void TypeConstraintGenerator::gather_function_outputs(const MatlabScope& scope,
                                                      const std::vector<MatlabIdentifier>& ids,
                                                      TypePtrs& into) {
  for (const auto& output : ids) {
    assert(output.full_name() >= 0);

    const auto variable_def_handle = scope.local_variables.at(output);
    const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

    into.push_back(variable_type_handle);
  }
}

void TypeConstraintGenerator::function_def_node(const FunctionDefNode& node) {
  ScopeState<const MatlabScope>::Helper matlab_scope_helper(scopes, node.scope);
  ScopeState<const TypeScope>::Helper type_scope_helper(type_scopes, node.type_scope);

  const auto input_handle = type_store.make_input_destructured_tuple(TypePtrs{});
  const auto output_handle = type_store.make_output_destructured_tuple(TypePtrs{});

  auto& function_inputs = MT_DT_MUT_REF(*input_handle).members;
  auto& function_outputs = MT_DT_MUT_REF(*output_handle).members;

  if (are_functions_polymorphic()) {
    push_constraint_repository();
  }

  MatlabIdentifier function_name;
  const Block* function_body = nullptr;
  FunctionAttributes function_attrs;

  const auto& scope = *scopes.current();

  store.use<Store::ReadConst>([&](const auto& reader) {
    const auto& def = reader.at(node.def_handle);
    function_name = def.header.name;

    gather_function_inputs(scope, def.header.inputs, function_inputs);
    gather_function_outputs(scope, def.header.outputs, function_outputs);

    function_body = def.body.get();
    function_attrs = def.attributes;
  });

  if (function_body) {
    //  Push a null handle to indicate that we're no longer directly inside a class.
    ClassDefState::Helper enclosing_class(class_state, ClassDefHandle{},
                                          nullptr, MatlabIdentifier());
    push_monomorphic_functions();
    function_body->accept_const(*this);
    pop_generic_function_state();
  }

  const auto type_handle =
    type_store.make_abstraction(function_name, node.ref_handle, input_handle, output_handle);
  const auto func_var = require_bound_type_variable(node.def_handle);

  auto rhs_term_type = type_handle;

  if (are_functions_polymorphic()) {
    auto current_constraints = std::move(current_constraint_repository());
    pop_constraint_repository();

    const auto func_scheme = type_store.make_scheme(type_handle, TypePtrs{});
    auto& scheme = MT_SCHEME_MUT_REF(*func_scheme);
    scheme.constraints = std::move(current_constraints.constraints);
    scheme.parameters = std::move(current_constraints.variables);

    rhs_term_type = func_scheme;
  }

  const auto lhs_term = make_term(&node.source_token, func_var);
  const auto rhs_term = make_term(&node.source_token, rhs_term_type);
  push_type_equation(make_eq(lhs_term, rhs_term));

  library.emplace_local_function_type(node.def_handle, rhs_term_type);

  // Maybe add this function as a method.
  if (class_state.is_within_class()) {
    handle_class_method(function_inputs, function_outputs, function_attrs, function_name,
                        type_handle, rhs_term);
  }

  push_type_equation_term(rhs_term);
}

void TypeConstraintGenerator::handle_class_method(const TypePtrs& function_inputs,
                                                  const TypePtrs& function_outputs,
                                                  const FunctionAttributes& function_attrs,
                                                  const MatlabIdentifier& function_name,
                                                  const Type* function_type_handle,
                                                  const TypeEquationTerm& rhs_term) {
  auto* class_type = class_state.enclosing_class_type();
  assert(class_type && function_type_handle->is_abstraction() && function_attrs.is_class_method());
  const auto& abstr = MT_ABSTR_REF(*function_type_handle);

  if (!function_attrs.is_static() && !function_attrs.is_constructor()) {
    //  First argument must be of class type.
    assert(!function_inputs.empty()); //  Should be previously handled during parse.
    const auto lhs_input_term = make_term(rhs_term.source_token, function_inputs[0]);
    const auto rhs_input_term = make_term(rhs_term.source_token, MT_TYPE_MUT_PTR(class_type));
    push_type_equation(make_eq(lhs_input_term, rhs_input_term));
  }

  if (function_attrs.is_constructor()) {
    assert(function_outputs.size() == 1); //  Should be previously handled during parse.
    const auto lhs_output_term = make_term(rhs_term.source_token, function_outputs[0]);
    const auto rhs_output_term = make_term(rhs_term.source_token, MT_TYPE_MUT_PTR(class_type));
    push_type_equation(make_eq(lhs_output_term, rhs_output_term));
  }

  Optional<BinaryOperator> maybe_binary_op;
  Optional<UnaryOperator> maybe_unary_op;

  if (!function_attrs.is_static()) {
    //  If this is a regular method, see if it's possibly an operator definition.
    const auto str_name = string_registry.at(function_name.full_name());
    maybe_binary_op = binary_operator_from_string(str_name);
    maybe_unary_op = unary_operator_from_string(str_name);
  }

  if (maybe_binary_op) {
    //  Binary operator
    auto bin_op = type_store.make_abstraction(maybe_binary_op.value(), abstr.inputs, abstr.outputs);
    auto bin_term = make_term(rhs_term.source_token, bin_op);
    push_type_equation(make_eq(bin_term, rhs_term));

    library.method_store.add_method(class_type, MT_ABSTR_REF(*bin_op), bin_op);

    if (represents_relation(maybe_binary_op.value()) && !function_outputs.empty()) {
      auto maybe_logical = library.get_logical_type();
      if (maybe_logical) {
        //  Must return logical.
        auto out_term = make_term(rhs_term.source_token, function_outputs[0]);
        auto log_term = make_term(rhs_term.source_token, maybe_logical.value());
        push_type_equation(make_eq(out_term, log_term));
      }
    }

  } else if (maybe_unary_op) {
    //  Unary operator
    auto un_op = type_store.make_abstraction(maybe_unary_op.value(), abstr.inputs, abstr.outputs);
    auto un_term = make_term(rhs_term.source_token, un_op);
    push_type_equation(make_eq(un_term, rhs_term));

    library.method_store.add_method(class_type, MT_ABSTR_REF(*un_op), un_op);
  }

  library.method_store.add_method(class_type, abstr, rhs_term.term);
}

void TypeConstraintGenerator::class_def_node(const ClassDefNode& node) {
  MatlabIdentifier name;
  store.use<Store::ReadConst>([&](const auto& reader) {
    name = reader.at(node.handle).name;
  });

  types::Record::Fields fields;
  for (const auto& prop : node.properties) {
    TypeIdentifier prop_name(prop.name);
    const auto name_type = type_store.make_constant_value(prop_name);
    const auto prop_type = make_fresh_type_variable_reference();
    fields.push_back(types::Record::Field{name_type, prop_type});

    if (prop.initializer) {
      auto init_term = visit_expr(prop.initializer, prop.source_token);
      auto prop_term = make_term(&prop.source_token, prop_type);
      push_type_equation(make_eq(init_term, prop_term));
    }
  }

  auto record_type = type_store.make_record(std::move(fields));
  auto maybe_class_type = type_scopes.current()->lookup_type(to_type_identifier(name));
  assert(maybe_class_type && maybe_class_type.value()->type->is_class());
  auto class_type = MT_CLASS_MUT_PTR(maybe_class_type.value()->type);
  assert(!class_type->source);
  class_type->source = record_type;

  ClassDefState::Helper class_helper(class_state, node.handle, class_type, name);

  for (const auto& method : node.method_defs) {
    method->accept_const(*this);
  }
}

/*
 * TypeAnnot
 */

void TypeConstraintGenerator::fun_type_node(const FunTypeNode& node) {
  push_polymorphic_functions();
  node.definition->accept_const(*this);
  pop_generic_function_state();
}

void TypeConstraintGenerator::type_annot_macro(const TypeAnnotMacro& macro) {
  macro.annotation->accept_const(*this);
}

void TypeConstraintGenerator::type_assertion(const TypeAssertion& assertion) {
  assertion.has_type->accept_const(*this);
  auto expected_type = pop_type_equation_term();

  assertion.node->accept_const(*this);
  auto actual_type = pop_type_equation_term();

  push_type_equation(make_eq(expected_type, actual_type));
}

void TypeConstraintGenerator::function_type_node(const FunctionTypeNode& node) {
  TypePtrs args;
  for (const auto& arg : node.inputs) {
    arg->accept_const(*this);
    args.push_back(pop_type_equation_term().term);
  }

  TypePtrs outputs;
  for (const auto& out : node.outputs) {
    out->accept_const(*this);
    outputs.push_back(pop_type_equation_term().term);
  }

  auto input_tup = type_store.make_input_destructured_tuple(std::move(args));
  auto output_tup = type_store.make_output_destructured_tuple(std::move(outputs));

  auto abstr = type_store.make_abstraction(input_tup, output_tup);
  auto term = make_term(&node.source_token, abstr);
  push_type_equation_term(term);
}

void TypeConstraintGenerator::scalar_type_node(const ScalarTypeNode& node) {
  assert(node.arguments.empty() && "Args not yet handled.");
  auto maybe_known_scalar_type = type_scopes.current()->lookup_type(node.identifier);
  assert(maybe_known_scalar_type && "Unknown scalar type.");
  auto type = maybe_known_scalar_type.value()->type;

  push_type_equation_term(make_term(&node.source_token, type));
}

/*
 * Expr
 */

void TypeConstraintGenerator::number_literal_expr(const NumberLiteralExpr& expr) {
  auto maybe_double = library.get_number_type();
  assert(maybe_double);
  push_type_equation_term(make_term(&expr.source_token, maybe_double.value()));
}

void TypeConstraintGenerator::char_literal_expr(const CharLiteralExpr& expr) {
  auto maybe_char = library.get_char_type();
  push_type_equation_term(make_term(&expr.source_token, maybe_char.value()));
}

void TypeConstraintGenerator::string_literal_expr(const StringLiteralExpr& expr) {
  auto maybe_str = library.get_string_type();
  push_type_equation_term(make_term(&expr.source_token, maybe_str.value()));
}

void TypeConstraintGenerator::dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) {
  auto maybe_char = library.get_char_type();
  assert(maybe_char);
  const auto field_term = visit_expr(expr.expr, expr.source_token);
  push_type_equation(make_eq(field_term, make_term(&expr.source_token, maybe_char.value())));
  push_type_equation_term(field_term);
}

void TypeConstraintGenerator::literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) {
  TypeIdentifier ident(expr.field_identifier);
  const auto const_type = type_store.make_constant_value(ident);
  push_type_equation_term(make_term(&expr.source_token, const_type));
}

void TypeConstraintGenerator::function_call_expr(const FunctionCallExpr& expr) {
  TypePtrs members;
  members.reserve(expr.arguments.size());

  for (const auto& arg : expr.arguments) {
    const auto result = visit_expr(arg, expr.source_token);
    members.push_back(result.term);
  }

  const auto ref = store.get(expr.reference_handle);

  const auto args_type = type_store.make_rvalue_destructured_tuple(std::move(members));
  auto result_type = make_fresh_type_variable_reference();
  const auto func_type = type_store.make_abstraction(ref.name, expr.reference_handle, args_type, result_type);

  const auto func_lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());

#if MT_SCHEME_FUNC_CALL
  const auto scheme_type = type_store.make_scheme(func_type, TypePtrs{});
  const auto func_rhs_term = make_term(&expr.source_token, scheme_type);
#else
  const auto func_rhs_term = make_term(&expr.source_token, func_type);
#endif

  push_type_equation(make_eq(func_lhs_term, func_rhs_term));

  if (ref.def_handle.is_valid()) {
    const auto local_func_var = require_bound_type_variable(ref.def_handle);
    const auto local_func_term = make_term(&expr.source_token, local_func_var);
    push_type_equation(make_eq(local_func_term, func_rhs_term));
  }

  push_type_equation_term(make_term(&expr.source_token, result_type));
}

void TypeConstraintGenerator::function_reference_expr(const FunctionReferenceExpr& expr) {
  auto ref = store.get(expr.handle);

  auto inputs = make_fresh_type_variable_reference();
  auto outputs = make_fresh_type_variable_reference();
  auto func = type_store.make_abstraction(ref.name, expr.handle, inputs, outputs);

  if (ref.def_handle.is_valid()) {
    const auto current_type = require_bound_type_variable(ref.def_handle);
    const auto lhs_term = make_term(&expr.source_token, current_type);

#if MT_SCHEME_FUNC_REF
    const auto scheme = type_store.make_scheme(func, TypePtrs{});
    const auto rhs_term = make_term(&expr.source_token, scheme);
#else
    const auto rhs_term = make_term(&expr.source_token, func);
#endif

    push_type_equation(make_eq(lhs_term, rhs_term));
  }

  push_type_equation_term(make_term(&expr.source_token, func));
}

void TypeConstraintGenerator::variable_reference_expr(const VariableReferenceExpr& expr) {
  using mt::types::Subscript;
  using mt::types::DestructuredTuple;
  using Use = DestructuredTuple::Usage;

  const auto variable_type_handle = require_bound_type_variable(expr.def_handle);
  const auto usage = value_category_state.is_lhs() ? Use::lvalue : Use::rvalue;

  if (expr.subscripts.empty()) {
    //  a = b; y = 1 + 2;
    const auto tup_handle = type_store.make_destructured_tuple(usage, variable_type_handle);
    push_type_equation_term(make_term(&expr.source_token, tup_handle));

  } else {
    std::vector<Subscript::Sub> subscripts;
    subscripts.reserve(expr.subscripts.size());

    value_category_state.push_rhs();

    for (const auto& sub : expr.subscripts) {
      auto method = sub.method;
      TypePtrs args;
      args.reserve(sub.arguments.size());

      for (const auto& arg_expr : sub.arguments) {
        const auto res = visit_expr(arg_expr, expr.source_token);
        args.push_back(res.term);
      }

      subscripts.emplace_back(Subscript::Sub(method, std::move(args)));
    }

    value_category_state.pop_side();

    const auto outputs_type = make_fresh_type_variable_reference();
    const auto sub_type = type_store.make_subscript(variable_type_handle, std::move(subscripts), outputs_type);

    const auto sub_lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());
    const auto sub_rhs_term = make_term(&expr.source_token, sub_type);

    push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));
    push_type_equation_term(make_term(&expr.source_token, outputs_type));
  }
}

void TypeConstraintGenerator::anonymous_function_expr(const AnonymousFunctionExpr& expr) {
  ScopeState<const MatlabScope>::Helper scope_helper(scopes, expr.scope);
  ScopeState<const TypeScope>::Helper type_scope_helper(type_scopes, expr.type_scope);

  //  Store constraints here.
  if (are_functions_polymorphic()) {
    push_constraint_repository();
  }

  TypePtrs function_inputs;
  TypePtrs function_outputs;

  gather_function_inputs(*scopes.current(), expr.inputs, function_inputs);

  const auto input_handle = type_store.make_input_destructured_tuple(std::move(function_inputs));
  const auto output_type = make_fresh_type_variable_reference();
  const auto output_term = make_term(&expr.source_token, output_type);

  const auto func = type_store.make_abstraction(input_handle, output_type);

  push_monomorphic_functions();
  const auto rhs_body_term = visit_expr(expr.expr, expr.source_token);
  pop_generic_function_state();

  push_type_equation(make_eq(rhs_body_term, output_term));

  if (are_functions_polymorphic()) {
    auto stored_constraints = std::move(current_constraint_repository());
    pop_constraint_repository();

    const auto func_scheme = type_store.make_scheme(func, TypePtrs{});
    const auto func_scheme_term = make_term(&expr.source_token, func_scheme);

    auto& scheme = MT_SCHEME_MUT_REF(*func_scheme);
    scheme.constraints = std::move(stored_constraints.constraints);
    scheme.parameters = std::move(stored_constraints.variables);

    push_type_equation_term(func_scheme_term);

  } else {
    push_type_equation_term(make_term(&expr.source_token, func));
  }
}

void TypeConstraintGenerator::superclass_method_reference_expr(const SuperclassMethodReferenceExpr& expr) {
  expr.superclass_reference_expr->accept_const(*this);
}

void TypeConstraintGenerator::binary_operator_expr(const BinaryOperatorExpr& expr) {
  const auto lhs_result = visit_expr(expr.left, expr.source_token);
  const auto rhs_result = visit_expr(expr.right, expr.source_token);

  const auto output_handle = make_fresh_type_variable_reference();
  const auto inputs_handle = type_store.make_rvalue_destructured_tuple(lhs_result.term, rhs_result.term);
  const auto func_handle = type_store.make_abstraction(expr.op, inputs_handle, output_handle);

  const auto func_lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());
  const auto func_rhs_term = make_term(&expr.source_token, func_handle);
  push_type_equation(make_eq(func_lhs_term, func_rhs_term));

  push_type_equation_term(make_term(&expr.source_token, output_handle));
}

void TypeConstraintGenerator::unary_operator_expr(const UnaryOperatorExpr& expr) {
  const auto lhs_result = visit_expr(expr.expr, expr.source_token);

  const auto output_handle = make_fresh_type_variable_reference();
  const auto inputs_handle = type_store.make_rvalue_destructured_tuple(lhs_result.term);
  const auto func_handle = type_store.make_abstraction(expr.op, inputs_handle, output_handle);

  const auto func_lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());
  const auto func_rhs_term = make_term(&expr.source_token, func_handle);
  push_type_equation(make_eq(func_lhs_term, func_rhs_term));

  push_type_equation_term(make_term(&expr.source_token, output_handle));
}

void TypeConstraintGenerator::grouping_expr(const GroupingExpr& expr) {
  if (expr.method == GroupingMethod::brace) {
    assert(!value_category_state.is_lhs());
    brace_grouping_expr_rhs(expr);

  } else if (expr.method == GroupingMethod::bracket) {
    if (value_category_state.is_lhs()) {
      bracket_grouping_expr_lhs(expr);
    } else {
      bracket_grouping_expr_rhs(expr);
    }

  } else if (expr.method == GroupingMethod::parens) {
    assert(!value_category_state.is_lhs());
    parens_grouping_expr_rhs(expr);

  } else {
    assert(false && "Unhandled grouping expr.");
  }
}

TypePtrs TypeConstraintGenerator::grouping_expr_components(const GroupingExpr& expr) {
  TypePtrs members;
  members.reserve(expr.components.size());

  for (const auto& component : expr.components) {
    const auto res = visit_expr(component.expr, expr.source_token);
    members.push_back(res.term);
  }

  return members;
}

void TypeConstraintGenerator::parens_grouping_expr_rhs(const GroupingExpr& expr) {
  auto members = grouping_expr_components(expr);
  const auto tup_type = type_store.make_rvalue_destructured_tuple(std::move(members));
  push_type_equation_term(make_term(&expr.source_token, tup_type));
}

void TypeConstraintGenerator::bracket_grouping_expr_lhs(const GroupingExpr& expr) {
  auto members = grouping_expr_components(expr);
  const auto tup_type = type_store.make_lvalue_destructured_tuple(std::move(members));
  push_type_equation_term(make_term(&expr.source_token, tup_type));
}

void TypeConstraintGenerator::bracket_grouping_expr_rhs(const GroupingExpr& expr) {
  auto last_dir = ConcatenationDirection::vertical;
  std::vector<TypePtrs> arg_handles;
  TypePtrs result_handles;

  arg_handles.emplace_back();
  result_handles.push_back(make_fresh_type_variable_reference());

  for (int64_t i = 0; i < int64_t(expr.components.size()); i++) {
    const auto& component = expr.components[i];
    const auto res = visit_expr(component.expr, expr.source_token);
    const auto current_dir = concatenation_direction_from_token_type(component.delimiter);

    if (i == 0 || last_dir == current_dir) {
      arg_handles.back().push_back(res.term);
    } else {
      assert(false && "Unhandled.");
    }

    last_dir = current_dir;
  }

  auto args = std::move(arg_handles.back());
  auto result_handle = result_handles.back();
  auto tup = type_store.make_rvalue_destructured_tuple(std::move(args));
  auto abstr = type_store.make_abstraction(last_dir, tup, result_handle);

  const auto lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());
  const auto rhs_term = make_term(&expr.source_token, abstr);
  push_type_equation(make_eq(lhs_term, rhs_term));

  push_type_equation_term(make_term(&expr.source_token, result_handle));
}

void TypeConstraintGenerator::brace_grouping_expr_rhs(const GroupingExpr& expr) {
  TypePtrs list_pattern;
  list_pattern.reserve(expr.components.size());

  for (const auto& component : expr.components) {
    const auto res = visit_expr(component.expr, expr.source_token);
    list_pattern.push_back(res.term);
  }

  const auto list_handle = type_store.make_list(std::move(list_pattern));
  const auto tuple_handle = type_store.make_tuple(list_handle);

  push_type_equation_term(make_term(&expr.source_token, tuple_handle));
}

/*
 * Stmt
 */

void TypeConstraintGenerator::expr_stmt(const ExprStmt& stmt) {
  stmt.expr->accept_const(*this);
}

void TypeConstraintGenerator::assignment_stmt(const AssignmentStmt& stmt) {
  assignment_state.push_assignment_target_rvalue();
  stmt.of_expr->accept_const(*this);
  assignment_state.pop_assignment_target_state();
  auto rhs = pop_type_equation_term();

  value_category_state.push_lhs();
  stmt.to_expr->accept_const(*this);
  value_category_state.pop_side();
  auto lhs = pop_type_equation_term();

  const auto assignment = type_store.make_assignment(lhs.term, rhs.term);
  const auto assignment_var = make_fresh_type_variable_reference();

  const auto lhs_term = make_term(lhs.source_token, assignment_var);
  const auto rhs_term = make_term(rhs.source_token, assignment);

  push_type_equation(make_eq(lhs_term, rhs_term));
}

void TypeConstraintGenerator::if_stmt(const IfStmt& stmt) {
  if_branch(stmt.if_branch);
  for (const auto& elseif_branch : stmt.elseif_branches) {
    if_branch(elseif_branch);
  }
  if (stmt.else_branch) {
    stmt.else_branch.value().block->accept_const(*this);
  }
}

void TypeConstraintGenerator::if_branch(const IfBranch& branch) {
  auto maybe_logical = library.get_logical_type();
  assert(maybe_logical);
  auto condition = visit_expr(branch.condition_expr, branch.source_token);
  auto lhs_term = make_term(&branch.source_token, maybe_logical.value());
  push_type_equation(make_eq(lhs_term, condition));

  branch.block->accept_const(*this);
}

void TypeConstraintGenerator::switch_stmt(const SwitchStmt& stmt) {
  //  @TODO: Handle case expressions with multiple brace-wrapped options: case {1, 2, 3}
  auto condition_term = visit_expr(stmt.condition_expr, stmt.source_token);

  for (const auto& switch_case : stmt.cases) {
    auto case_term = visit_expr(switch_case.expr, switch_case.source_token);
    push_type_equation(make_eq(condition_term, case_term));

    switch_case.block->accept_const(*this);
  }

  if (stmt.otherwise) {
    stmt.otherwise->accept_const(*this);
  }
}

void TypeConstraintGenerator::try_stmt(const TryStmt& stmt) {
  stmt.try_block->accept_const(*this);

  if (stmt.catch_block) {
    const auto& catch_block = stmt.catch_block.value();

    if (catch_block.expr) {
      (void) visit_expr(catch_block.expr, catch_block.source_token);
    }

    catch_block.block->accept_const(*this);
  }
}

/*
 * Util
 */

TypeEquationTerm TypeConstraintGenerator::visit_expr(const BoxedExpr& expr, const Token& source_token) {
  const auto lhs_term = make_term(&source_token, make_fresh_type_variable_reference());
  push_type_equation_term(lhs_term);
  expr->accept_const(*this);
  auto rhs_term = pop_type_equation_term();
  push_type_equation(make_eq(lhs_term, rhs_term));

  return rhs_term;
}

Type* TypeConstraintGenerator::make_fresh_type_variable_reference() {
  auto var_handle = type_store.make_fresh_type_variable_reference();
  if (!constraint_repositories.empty()) {
    constraint_repositories.back().variables.push_back(var_handle);
  }
  return var_handle;
}

Type* TypeConstraintGenerator::require_bound_type_variable(const VariableDefHandle& variable_def_handle) {
  if (variable_type_handles.count(variable_def_handle) > 0) {
    return variable_type_handles.at(variable_def_handle);
  }

  const auto variable_type_handle = make_fresh_type_variable_reference();
  bind_type_variable_to_variable_def(variable_def_handle, variable_type_handle);

  return variable_type_handle;
}

void TypeConstraintGenerator::bind_type_variable_to_variable_def(const VariableDefHandle& def_handle, Type* type_handle) {
  variable_type_handles[def_handle] = type_handle;
  variables[type_handle] = def_handle;
}

void TypeConstraintGenerator::bind_type_variable_to_function_def(const FunctionDefHandle& def_handle, Type* type_handle) {
  function_type_handles[def_handle] = type_handle;
}

Type* TypeConstraintGenerator::require_bound_type_variable(const FunctionDefHandle& function_def_handle) {
  if (function_type_handles.count(function_def_handle) > 0) {
    return function_type_handles.at(function_def_handle);
  }

  const auto function_type_handle = make_fresh_type_variable_reference();
  bind_type_variable_to_function_def(function_def_handle, function_type_handle);

  return function_type_handle;
}

void TypeConstraintGenerator::push_type_equation(const TypeEquation& eq) {
  if (constraint_repositories.empty()) {
    substitution.push_type_equation(eq);
  } else {
    substitution.push_type_equation(eq);
    constraint_repositories.back().constraints.push_back(eq);
  }
}

void TypeConstraintGenerator::push_monomorphic_functions() {
  are_functions_generic.push_back(false);
}

void TypeConstraintGenerator::push_polymorphic_functions() {
  are_functions_generic.push_back(true);
}

void TypeConstraintGenerator::pop_generic_function_state() {
  assert(!are_functions_generic.empty());
  are_functions_generic.pop_back();
}

bool TypeConstraintGenerator::are_functions_polymorphic() const {
  assert(!are_functions_generic.empty());
  return are_functions_generic.back();
}

void TypeConstraintGenerator::push_constraint_repository() {
  constraint_repositories.emplace_back();
}

TypeConstraintGenerator::ConstraintRepository& TypeConstraintGenerator::current_constraint_repository() {
  assert(!constraint_repositories.empty());
  return constraint_repositories.back();
}

void TypeConstraintGenerator::pop_constraint_repository() {
  assert(!constraint_repositories.empty() && "No constraints to pop.");
  constraint_repositories.pop_back();
}

void TypeConstraintGenerator::push_type_equation_term(const TypeEquationTerm& term) {
  type_eq_terms.push_back(term);
}

TypeEquationTerm TypeConstraintGenerator::pop_type_equation_term() {
  assert(!type_eq_terms.empty());
  const auto term = type_eq_terms.back();
  type_eq_terms.pop_back();
  return term;
}

}