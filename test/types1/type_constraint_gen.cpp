#include "type_constraint_gen.hpp"
#include "debug.hpp"
#include "type_representation.hpp"

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
  MatlabScopeHelper scope_helper(*this, block.scope_handle);
  block.block->accept_const(*this);
}

void TypeConstraintGenerator::block(const Block& block) {
  for (const auto& node : block.nodes) {
    node->accept_const(*this);
  }
}

void TypeConstraintGenerator::show_variable_types(const TypeToString& printer) const {
  std::cout << "--" << std::endl;

  Store::ReadConst reader(store);

  for (const auto& var_it : variable_type_handles) {
    const auto& maybe_type = substitution.bound_type(var_it.second);
    const auto& type = maybe_type ? maybe_type.value() : var_it.second;

    const auto& def_handle = var_it.first;
    const auto& def = reader.at(def_handle);
    const auto& name = string_registry.at(def.name.full_name());

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
  using namespace mt::types;

  const Block* function_body = nullptr;
  MatlabScopeHelper scope_helper(*this, node.scope_handle);

  const auto type_handle = type_store.make_type();
  bind_type_variable_to_function_def(node.def_handle, type_handle);

  std::vector<TypeHandle> function_inputs;
  std::vector<TypeHandle> function_outputs;

  MatlabIdentifier function_name;

  {
    const auto scope_handle = current_scope_handle();

    Store::ReadConst reader(store);
    const auto& def = reader.at(node.def_handle);
    const auto& scope = reader.at(scope_handle);
    function_name = def.header.name;

    gather_function_inputs(scope, def.header.inputs, function_inputs);
    gather_function_outputs(scope, def.header.outputs, function_outputs);

    function_body = def.body.get();
  }

  const auto input_handle = type_store.make_input_destructured_tuple(std::move(function_inputs));
  const auto output_handle = type_store.make_output_destructured_tuple(std::move(function_outputs));

  const auto lhs_term = make_term(&node.source_token, make_fresh_type_variable_reference());
  const auto rhs_term = make_term(&node.source_token, type_handle);

  type_store.assign(type_handle, Type(Abstraction(function_name, input_handle, output_handle)));
  push_type_equation(make_eq(lhs_term, rhs_term));

  if (function_body) {
    function_body->accept_const(*this);
  }
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

  const auto args_type = type_store.make_rvalue_destructured_tuple(std::move(members));
  auto func_type = type_store.make_type();
  auto result_type = make_fresh_type_variable_reference();

  MatlabIdentifier function_name;
  {
    Store::ReadConst reader(store);
    function_name = reader.at(expr.reference_handle).name;
  }

  const auto func_lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());
  const auto func_rhs_term = make_term(&expr.source_token, func_type);

  type_store.assign(func_type, Type(types::Abstraction(function_name, args_type, result_type)));
  push_type_equation(make_eq(func_lhs_term, func_rhs_term));

  push_type_equation_term(make_term(&expr.source_token, result_type));
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

    auto sub_type = type_store.make_type();
    auto outputs_type = make_fresh_type_variable_reference();

    const auto sub_lhs_term = make_term(&expr.source_token, make_fresh_type_variable_reference());
    const auto sub_rhs_term = make_term(&expr.source_token, sub_type);

    type_store.assign(sub_type, Type(Subscript(variable_type_handle, std::move(subscripts), outputs_type)));
    push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

    push_type_equation_term(make_term(&expr.source_token, outputs_type));
  }
}

void TypeConstraintGenerator::function_reference_expr(const FunctionReferenceExpr& expr) {
  Store::ReadConst reader(store);
  const auto& ref = reader.at(expr.handle);
  assert(!ref.def_handle.is_valid() && "Local funcs not yet handled.");

  auto inputs = make_fresh_type_variable_reference();
  auto outputs = make_fresh_type_variable_reference();
  auto func = type_store.make_abstraction(ref.name, inputs, outputs);

  push_type_equation_term(make_term(&expr.source_token, func));
}

void TypeConstraintGenerator::anonymous_function_expr(const AnonymousFunctionExpr& expr) {
  using namespace mt::types;

  MatlabScopeHelper scope_helper(*this, expr.scope_handle);

  //  Store constraints here.
  push_constraint_repository();

  std::vector<TypeHandle> function_inputs;
  std::vector<TypeHandle> function_outputs;

  {
    const auto scope_handle = current_scope_handle();

    Store::ReadConst reader(store);
    const auto& scope = reader.at(scope_handle);

    gather_function_inputs(scope, expr.inputs, function_inputs);
  }

  const auto input_handle = type_store.make_input_destructured_tuple(std::move(function_inputs));
  const auto output_type = make_fresh_type_variable_reference();
  const auto output_term = make_term(&expr.source_token, output_type);

  const auto func = type_store.make_abstraction(input_handle, output_type);

  const auto lhs_body_term = make_term(&expr.source_token, make_fresh_type_variable_reference());
  push_type_equation_term(lhs_body_term);
  expr.expr->accept_const(*this);
  const auto rhs_body_term = pop_type_equation_term();

  auto stored_constraints = std::move(current_constraint_repository());
  pop_constraint_repository();

  auto& scheme_variables = stored_constraints.variables;
  const auto func_scheme = type_store.make_scheme(func, std::move(scheme_variables));
  const auto func_scheme_term = make_term(&expr.source_token, func_scheme);

  push_type_equation(make_eq(lhs_body_term, rhs_body_term));
  push_type_equation(make_eq(rhs_body_term, output_term));

  type_store.at(func_scheme).scheme.constraints = std::move(stored_constraints.constraints);

  push_type_equation_term(func_scheme_term);
}

void TypeConstraintGenerator::binary_operator_expr(const BinaryOperatorExpr& expr) {
  using namespace mt::types;

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
  using types::List;
  using types::Tuple;

  List list;
  list.pattern.reserve(expr.components.size());

  for (const auto& component : expr.components) {
    const auto res = visit_expr(component.expr, expr.source_token);
    list.pattern.push_back(res.term);
  }

  auto list_handle = type_store.make_type();
  auto tuple_handle = type_store.make_type();

  type_store.assign(list_handle, Type(std::move(list)));
  type_store.assign(tuple_handle, Type(Tuple(list_handle)));

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

}