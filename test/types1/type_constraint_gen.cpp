#include "mt/display.hpp"
#include "type_constraint_gen.hpp"
#include "debug.hpp"
#include "type_representation.hpp"

#define MT_SCHEME_FUNC_REF (1)
#define MT_SCHEME_FUNC_CALL (1)

namespace mt {

void TypeConstraintGenerator::show_type_distribution() const {
  auto counts = type_store.type_distribution();
  const auto num_types = double(counts.size());

  for (const auto& ct : counts) {
    std::cout << to_string(ct.first) << ": " << ct.second << " (";
    std::cout << ct.second/num_types << ")" << std::endl;
  }
}

void TypeConstraintGenerator::root_block(const RootBlock& block) {
  MatlabScopeHelper scope_helper(*this, block.scope);
  block.block->accept_const(*this);
}

void TypeConstraintGenerator::block(const Block& block) {
  for (const auto& node : block.nodes) {
    node->accept_const(*this);
  }
}

void TypeConstraintGenerator::show_local_function_types(const TypeToString& printer) const {
  std::cout << "--" << std::endl;

  for (const auto& func_it : function_type_handles) {
    const auto& def_handle = func_it.first;
    const auto& maybe_type = substitution.bound_type(func_it.second);
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
  std::cout << "--" << std::endl;

  for (const auto& var_it : variable_type_handles) {
    const auto& def_handle = var_it.first;
    const auto& maybe_type = substitution.bound_type(var_it.second);
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
                                                     TypeHandles& into) {
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
                                                      TypeHandles& into) {
  for (const auto& output : ids) {
    assert(output.full_name() >= 0);

    const auto variable_def_handle = scope.local_variables.at(output);
    const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

    into.push_back(variable_type_handle);
  }
}

void TypeConstraintGenerator::function_def_node(const FunctionDefNode& node) {
  MatlabScopeHelper scope_helper(*this, node.scope);

  const auto input_handle = type_store.make_input_destructured_tuple(TypeHandles{});
  const auto output_handle = type_store.make_output_destructured_tuple(TypeHandles{});

  auto& function_inputs = type_store.at(input_handle).destructured_tuple.members;
  auto& function_outputs = type_store.at(output_handle).destructured_tuple.members;

  if (are_functions_polymorphic()) {
    push_constraint_repository();
  }

  MatlabIdentifier function_name;
  const Block* function_body = nullptr;
  const auto& scope = *current_scope();

  store.use<Store::ReadConst>([&](const auto& reader) {
    const auto& def = reader.at(node.def_handle);
    function_name = def.header.name;

    gather_function_inputs(scope, def.header.inputs, function_inputs);
    gather_function_outputs(scope, def.header.outputs, function_outputs);

    function_body = def.body.get();
  });

  if (function_body) {
    push_monomorphic_functions();
    function_body->accept_const(*this);
    pop_generic_function_state();
  }

  const auto type_handle =
    type_store.make_abstraction(function_name, node.def_handle, input_handle, output_handle);
  const auto func_var = require_bound_type_variable(node.def_handle);

  TypeHandle rhs_term_type = type_handle;

  if (are_functions_polymorphic()) {
    auto current_constraints = std::move(current_constraint_repository());
    pop_constraint_repository();

    const auto func_scheme = type_store.make_scheme(type_handle, TypeHandles{});
    auto& scheme = type_store.at(func_scheme).scheme;
    scheme.constraints = std::move(current_constraints.constraints);
    scheme.parameters = std::move(current_constraints.variables);

    rhs_term_type = func_scheme;
  }

  library.emplace_local_function_type(node.def_handle, rhs_term_type);

  const auto lhs_term = make_term(&node.source_token, func_var);
  const auto rhs_term = make_term(&node.source_token, rhs_term_type);
  push_type_equation(make_eq(lhs_term, rhs_term));
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

/*
 * Expr
 */

void TypeConstraintGenerator::number_literal_expr(const NumberLiteralExpr& expr) {
  assert(library.double_type_handle.is_valid());
  push_type_equation_term(make_term(&expr.source_token, library.double_type_handle));
}

void TypeConstraintGenerator::char_literal_expr(const CharLiteralExpr& expr) {
  assert(library.char_type_handle.is_valid());
  push_type_equation_term(make_term(&expr.source_token, library.char_type_handle));
}

void TypeConstraintGenerator::string_literal_expr(const StringLiteralExpr& expr) {
  assert(library.string_type_handle.is_valid());
  push_type_equation_term(make_term(&expr.source_token, library.string_type_handle));
}

void TypeConstraintGenerator::dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) {
  const auto field_term = visit_expr(expr.expr, expr.source_token);
  push_type_equation(make_eq(field_term, make_term(&expr.source_token, library.char_type_handle)));
  push_type_equation_term(field_term);
}

void TypeConstraintGenerator::literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) {
  assert(library.char_type_handle.is_valid());
  push_type_equation_term(make_term(&expr.source_token, library.char_type_handle));
}

void TypeConstraintGenerator::function_call_expr(const FunctionCallExpr& expr) {
  std::vector<TypeHandle> members;
  members.reserve(expr.arguments.size());

  for (const auto& arg : expr.arguments) {
    const auto result = visit_expr(arg, expr.source_token);
    members.push_back(result.term);
  }

  MatlabIdentifier function_name;
  FunctionDefHandle def_handle;

  store.use<Store::ReadConst>([&](const auto& reader) {
    const auto& ref = reader.at(expr.reference_handle);
    def_handle = ref.def_handle;
    function_name = ref.name;
  });

  const auto args_type = type_store.make_rvalue_destructured_tuple(std::move(members));
  auto result_type = make_fresh_type_variable_reference();
  const auto func_type = type_store.make_abstraction(function_name, def_handle, args_type, result_type);

  const auto func_lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());

#if MT_SCHEME_FUNC_CALL
  const auto scheme_type = type_store.make_scheme(func_type, TypeHandles{});
  const auto func_rhs_term = make_term(&expr.source_token, scheme_type);
#else
  const auto func_rhs_term = make_term(&expr.source_token, func_type);
#endif

  push_type_equation(make_eq(func_lhs_term, func_rhs_term));

  if (def_handle.is_valid()) {
    const auto local_func_var = require_bound_type_variable(def_handle);
    const auto local_func_term = make_term(&expr.source_token, local_func_var);
    push_type_equation(make_eq(local_func_term, func_rhs_term));
  }

  push_type_equation_term(make_term(&expr.source_token, result_type));
}

void TypeConstraintGenerator::function_reference_expr(const FunctionReferenceExpr& expr) {
  MatlabIdentifier name;
  FunctionDefHandle def_handle;

  store.use<Store::ReadConst>([&](const auto& reader) {
    const auto& ref = reader.at(expr.handle);
    def_handle = ref.def_handle;
    name = ref.name;
  });

  auto inputs = make_fresh_type_variable_reference();
  auto outputs = make_fresh_type_variable_reference();
  auto func = type_store.make_abstraction(name, def_handle, inputs, outputs);

  if (def_handle.is_valid()) {
    const auto current_type = require_bound_type_variable(def_handle);
    const auto lhs_term = make_term(&expr.source_token, current_type);

#if MT_SCHEME_FUNC_REF
    const auto scheme = type_store.make_scheme(func, TypeHandles{});
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
      std::vector<TypeHandle> args;
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
  MatlabScopeHelper scope_helper(*this, expr.scope);

  //  Store constraints here.
  if (are_functions_polymorphic()) {
    push_constraint_repository();
  }

  std::vector<TypeHandle> function_inputs;
  std::vector<TypeHandle> function_outputs;

  gather_function_inputs(*current_scope(), expr.inputs, function_inputs);

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

    const auto func_scheme = type_store.make_scheme(func, TypeHandles{});
    const auto func_scheme_term = make_term(&expr.source_token, func_scheme);

    auto& scheme = type_store.at(func_scheme).scheme;
    scheme.constraints = std::move(stored_constraints.constraints);
    scheme.parameters = std::move(stored_constraints.variables);

    push_type_equation_term(func_scheme_term);

  } else {
    push_type_equation_term(make_term(&expr.source_token, func));
  }
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

std::vector<TypeHandle> TypeConstraintGenerator::grouping_expr_components(const GroupingExpr& expr) {
  std::vector<TypeHandle> members;
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
  std::vector<TypeHandles> arg_handles;
  std::vector<TypeHandle> result_handles;

  arg_handles.emplace_back();
  result_handles.push_back(make_fresh_type_variable_reference());

  for (int64_t i = 0; i < expr.components.size(); i++) {
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
  TypeHandles list_pattern;
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

/*
 * Stmt
 */

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
  auto condition = visit_expr(branch.condition_expr, branch.source_token);
  auto lhs_term = make_term(&branch.source_token, library.logical_type_handle);
  push_type_equation(make_eq(lhs_term, condition));

  branch.block->accept_const(*this);
}

void TypeConstraintGenerator::switch_stmt(const SwitchStmt& stmt) {
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

TypeHandle TypeConstraintGenerator::make_fresh_type_variable_reference() {
  auto var_handle = type_store.make_fresh_type_variable_reference();
  if (!constraint_repositories.empty()) {
    constraint_repositories.back().variables.push_back(var_handle);
  }
  return var_handle;
}

TypeHandle TypeConstraintGenerator::require_bound_type_variable(const VariableDefHandle& variable_def_handle) {
  if (variable_type_handles.count(variable_def_handle) > 0) {
    return variable_type_handles.at(variable_def_handle);
  }

  const auto variable_type_handle = make_fresh_type_variable_reference();
  bind_type_variable_to_variable_def(variable_def_handle, variable_type_handle);

  return variable_type_handle;
}

void TypeConstraintGenerator::bind_type_variable_to_variable_def(const VariableDefHandle& def_handle, TypeRef type_handle) {
  variable_type_handles[def_handle] = type_handle;
  variables[type_handle] = def_handle;
}

void TypeConstraintGenerator::bind_type_variable_to_function_def(const FunctionDefHandle& def_handle, TypeRef type_handle) {
  function_type_handles[def_handle] = type_handle;
  functions[type_handle] = def_handle;
}

TypeHandle TypeConstraintGenerator::require_bound_type_variable(const FunctionDefHandle& function_def_handle) {
  if (function_type_handles.count(function_def_handle) > 0) {
    return function_type_handles.at(function_def_handle);
  }

  const auto function_type_handle = make_fresh_type_variable_reference();
  bind_type_variable_to_function_def(function_def_handle, function_type_handle);

  return function_type_handle;
}

}