#include "typing.hpp"
#include "debug.hpp"
#include "type_representation.hpp"

namespace mt {

void TypeVisitor::show_type_distribution() const {
  auto counts = type_store.type_distribution();
  const auto num_types = double(counts.size());

  for (const auto& ct : counts) {
    std::cout << to_string(ct.first) << ": " << ct.second << " (";
    std::cout << ct.second/num_types << ")" << std::endl;
  }
}

void TypeVisitor::show_variable_types(TypeToString& printer) const {
  std::cout << "--" << std::endl;

  Store::ReadConst reader(store);

  for (const auto& var_it : variable_type_handles) {
    const auto& maybe_type = substitution.bound_type(var_it.second);
    const Type& type = maybe_type ? type_store.at(maybe_type.value()) : type_store.at(var_it.second);

    const auto& def_handle = var_it.first;
    const auto& def = reader.at(def_handle);
    const auto& name = string_registry.at(def.name.full_name());

    printer.clear();
    printer.apply(type);
    auto type_str = printer.str();
    printer.clear();

    std::cout << name << ": " << type_str << std::endl;
  }
}

void TypeVisitor::gather_function_inputs(const MatlabScope& scope, const std::vector<FunctionInputParameter>& inputs, TypeHandles& into) {
  for (const auto& input : inputs) {
    if (!input.is_ignored) {
      assert(input.name.full_name() >= 0);

      const auto variable_def_handle = scope.local_variables.at(input.name);
      const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

      into.push_back(variable_type_handle);
    } else {
      //  @TODO: Change this type to Top
      into.push_back(type_store.make_fresh_type_variable_reference());
    }
  }
}

void TypeVisitor::function_def_node(const FunctionDefNode& node) {
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

    for (const auto& output : def.header.outputs) {
      assert(output.full_name() >= 0);

      const auto variable_def_handle = scope.local_variables.at(output);
      const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

      function_outputs.push_back(variable_type_handle);
    }

    function_body = def.body.get();
  }

  const auto input_handle = type_store.make_type();
  const auto output_handle = type_store.make_type();

  type_store.assign(input_handle,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, std::move(function_inputs))));
  type_store.assign(output_handle,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, std::move(function_outputs))));

  const auto lhs_term = make_term(&node.source_token, type_store.make_fresh_type_variable_reference());
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

void TypeVisitor::function_call_expr(const FunctionCallExpr& expr) {
  using types::DestructuredTuple;
  using types::Abstraction;
  using Use = DestructuredTuple::Usage;

  std::vector<TypeHandle> members;
  members.reserve(expr.arguments.size());

  for (const auto& arg : expr.arguments) {
    const auto tvar = type_store.make_fresh_type_variable_reference();
    const auto tvar_term = make_term(&expr.source_token, tvar);
    push_type_equation_term(tvar_term);
    arg->accept_const(*this);
    auto result = pop_type_equation_term();

    push_type_equation(make_eq(tvar_term, result));
    members.push_back(result.term);
  }

  auto args_type = type_store.make_type();
  auto func_type = type_store.make_type();
  auto result_type = type_store.make_fresh_type_variable_reference();

  DestructuredTuple tup(Use::rvalue, std::move(members));
  type_store.assign(args_type, Type(std::move(tup)));

  MatlabIdentifier function_name;
  {
    Store::ReadConst reader(store);
    function_name = reader.at(expr.reference_handle).name;
  }

  const auto func_lhs_term = make_term(&expr.source_token, type_store.make_fresh_type_variable_reference());
  const auto func_rhs_term = make_term(&expr.source_token, func_type);

  type_store.assign(func_type, Type(Abstraction(function_name, args_type, result_type)));
  push_type_equation(make_eq(func_lhs_term, func_rhs_term));

  push_type_equation_term(TypeEquationTerm(&expr.source_token, result_type));
}

void TypeVisitor::variable_reference_expr(const VariableReferenceExpr& expr) {
  using mt::types::Subscript;
  using mt::types::DestructuredTuple;
  using Use = DestructuredTuple::Usage;

  const auto variable_type_handle = require_bound_type_variable(expr.def_handle);
  const auto usage = value_category_state.is_lhs() ? Use::lvalue : Use::rvalue;

  if (expr.subscripts.empty()) {
    //  a = b; y = 1 + 2;
    auto tup_handle = type_store.make_type();
    type_store.assign(tup_handle, Type(DestructuredTuple(usage, variable_type_handle)));
    push_type_equation_term(TypeEquationTerm(&expr.source_token, tup_handle));

  } else {
    std::vector<Subscript::Sub> subscripts;
    subscripts.reserve(expr.subscripts.size());

    value_category_state.push_rhs();

    for (const auto& sub : expr.subscripts) {
      auto method = sub.method;
      std::vector<TypeHandle> args;
      args.reserve(sub.arguments.size());

      for (const auto& arg_expr : sub.arguments) {
        auto var_handle = type_store.make_fresh_type_variable_reference();
        const auto var_term = make_term(&expr.source_token, var_handle);
        push_type_equation_term(var_term);
        arg_expr->accept_const(*this);
        auto res = pop_type_equation_term();
        args.push_back(res.term);

        push_type_equation(make_eq(var_term, res));
      }

      subscripts.emplace_back(Subscript::Sub(method, std::move(args)));
    }

    value_category_state.pop_side();

    auto sub_type = type_store.make_type();
    auto outputs_type = type_store.make_fresh_type_variable_reference();

    const auto sub_lhs_term = make_term(&expr.source_token, type_store.make_fresh_type_variable_reference());
    const auto sub_rhs_term = make_term(&expr.source_token, sub_type);

    type_store.assign(sub_type, Type(Subscript(variable_type_handle, std::move(subscripts), outputs_type)));
    push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

    push_type_equation_term(TypeEquationTerm(&expr.source_token, outputs_type));
  }
}

void TypeVisitor::function_reference_expr(const FunctionReferenceExpr& expr) {
  Store::ReadConst reader(store);
  const auto& ref = reader.at(expr.handle);
  assert(!ref.def_handle.is_valid() && "Local funcs not yet handled.");

  auto inputs = type_store.make_fresh_type_variable_reference();
  auto outputs = type_store.make_fresh_type_variable_reference();
  auto func = type_store.make_abstraction(ref.name, inputs, outputs);

  push_type_equation_term(TypeEquationTerm(&expr.source_token, func));
}

void TypeVisitor::anonymous_function_expr(const AnonymousFunctionExpr& expr) {
  using namespace mt::types;

  MatlabScopeHelper scope_helper(*this, expr.scope_handle);

  std::vector<TypeHandle> function_inputs;
  std::vector<TypeHandle> function_outputs;

  {
    const auto scope_handle = current_scope_handle();

    Store::ReadConst reader(store);
    const auto& scope = reader.at(scope_handle);

    gather_function_inputs(scope, expr.inputs, function_inputs);
  }

  const auto input_handle = type_store.make_type();
  const auto output_type = type_store.make_fresh_type_variable_reference();
  const auto output_term = make_term(&expr.source_token, output_type);

  const auto func = type_store.make_abstraction(input_handle, output_type);
  const auto func_term = make_term(&expr.source_token, func);
  auto scheme_variables = function_inputs;
  scheme_variables.push_back(output_type);

  type_store.assign(input_handle,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, std::move(function_inputs))));

  //  Store constraints here.
  push_constraint_repository();

  const auto lhs_body_term = make_term(&expr.source_token, type_store.make_fresh_type_variable_reference());
  push_type_equation_term(lhs_body_term);
  expr.expr->accept_const(*this);
  const auto rhs_body_term = pop_type_equation_term();

  const auto func_scheme = type_store.make_scheme(func, std::move(scheme_variables));
  const auto func_scheme_term = make_term(&expr.source_token, func_scheme);

  push_type_equation(make_eq(lhs_body_term, rhs_body_term));
  push_type_equation(make_eq(rhs_body_term, output_term));

  auto stored_constraints = std::move(current_constraint_repository());
  pop_constraint_repository();

  type_store.at(func_scheme).scheme.constraints = std::move(stored_constraints);

//  push_type_equation_term(func_term);
  push_type_equation_term(func_scheme_term);
}

void TypeVisitor::binary_operator_expr(const BinaryOperatorExpr& expr) {
  using namespace mt::types;

  const auto tvar_lhs = type_store.make_fresh_type_variable_reference();
  const auto tvar_lhs_term = make_term(&expr.source_token, tvar_lhs);
  push_type_equation_term(tvar_lhs_term);
  expr.left->accept_const(*this);
  auto lhs_result = pop_type_equation_term();

  if (variables.count(lhs_result.term) == 0) {
    push_type_equation(make_eq(tvar_lhs_term, lhs_result));
  }

  const auto tvar_rhs = type_store.make_fresh_type_variable_reference();
  const auto tvar_rhs_term = make_term(&expr.source_token, tvar_rhs);
  push_type_equation_term(tvar_rhs_term);
  expr.right->accept_const(*this);
  auto rhs_result = pop_type_equation_term();

  if (variables.count(rhs_result.term) == 0) {
    push_type_equation(make_eq(tvar_rhs_term, rhs_result));
  }

  const auto output_handle = type_store.make_fresh_type_variable_reference();
  const auto inputs_handle = type_store.make_type();

  type_store.assign(inputs_handle,
    Type(DestructuredTuple(DestructuredTuple::Usage::rvalue, lhs_result.term, rhs_result.term)));

  types::Abstraction func_type(expr.op, inputs_handle, output_handle);
  const auto func_handle = type_store.make_type();
  type_store.assign(func_handle, Type(std::move(func_type)));

  const auto func_lhs_term = make_term(&expr.source_token, type_store.make_fresh_type_variable_reference());
  const auto func_rhs_term = make_term(&expr.source_token, func_handle);
  push_type_equation(make_eq(func_lhs_term, func_rhs_term));

  push_type_equation_term(TypeEquationTerm(&expr.source_token, output_handle));
}

void TypeVisitor::grouping_expr(const GroupingExpr& expr) {
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

std::vector<TypeHandle> TypeVisitor::grouping_expr_components(const GroupingExpr& expr) {
  std::vector<TypeHandle> members;
  members.reserve(expr.components.size());

  for (const auto& component : expr.components) {
    auto tvar = type_store.make_fresh_type_variable_reference();
    const auto tvar_term = make_term(&expr.source_token, tvar);
    push_type_equation_term(tvar_term);
    component.expr->accept_const(*this);
    auto res = pop_type_equation_term();

    push_type_equation(make_eq(tvar_term, res));
    members.push_back(res.term);
  }

  return members;
}

void TypeVisitor::parens_grouping_expr_rhs(const GroupingExpr& expr) {
  using types::DestructuredTuple;
  using Use = DestructuredTuple::Usage;

  auto tup_type = type_store.make_type();
  auto members = grouping_expr_components(expr);

  type_store.assign(tup_type, Type(DestructuredTuple(Use::rvalue, std::move(members))));
  push_type_equation_term(TypeEquationTerm(&expr.source_token, tup_type));
}

void TypeVisitor::bracket_grouping_expr_lhs(const GroupingExpr& expr) {
  using types::DestructuredTuple;
  using Use = DestructuredTuple::Usage;

  auto tup_type = type_store.make_type();
  auto members = grouping_expr_components(expr);

  type_store.assign(tup_type, Type(DestructuredTuple(Use::lvalue, std::move(members))));
  push_type_equation_term(TypeEquationTerm(&expr.source_token, tup_type));
}

void TypeVisitor::bracket_grouping_expr_rhs(const GroupingExpr& expr) {
  auto last_dir = ConcatenationDirection::vertical;
  std::vector<TypeHandles> arg_handles;
  std::vector<TypeHandle> result_handles;

  arg_handles.emplace_back();
  result_handles.push_back(type_store.make_fresh_type_variable_reference());

  for (int64_t i = 0; i < expr.components.size(); i++) {
    const auto& component = expr.components[i];
    auto tvar = type_store.make_fresh_type_variable_reference();
    push_type_equation_term(TypeEquationTerm(&expr.source_token, tvar));
    component.expr->accept_const(*this);
    auto res = pop_type_equation_term();

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
  auto tup = type_store.make_destructured_tuple(types::DestructuredTuple::Usage::rvalue, std::move(args));
  auto abstr = type_store.make_abstraction(last_dir, tup, result_handle);

  const auto lhs_term = make_term(&expr.source_token, type_store.make_fresh_type_variable_reference());
  const auto rhs_term = make_term(&expr.source_token, abstr);
  push_type_equation(make_eq(lhs_term, rhs_term));

  push_type_equation_term(TypeEquationTerm(&expr.source_token, result_handle));
}

void TypeVisitor::brace_grouping_expr_rhs(const GroupingExpr& expr) {
  using types::List;
  using types::Tuple;

  List list;
  list.pattern.reserve(expr.components.size());

  for (const auto& component : expr.components) {
    auto tvar = type_store.make_fresh_type_variable_reference();
    const auto tvar_term = make_term(&expr.source_token, tvar);
    push_type_equation_term(tvar_term);
    component.expr->accept_const(*this);
    auto res = pop_type_equation_term();

    push_type_equation(make_eq(tvar_term, res));
    list.pattern.push_back(res.term);
  }

  auto list_handle = type_store.make_type();
  auto tuple_handle = type_store.make_type();

  type_store.assign(list_handle, Type(std::move(list)));
  type_store.assign(tuple_handle, Type(Tuple(list_handle)));

  push_type_equation_term(TypeEquationTerm(&expr.source_token, tuple_handle));
}

/*
 * Stmt
 */

void TypeVisitor::expr_stmt(const ExprStmt& stmt) {
  stmt.expr->accept_const(*this);
}

void TypeVisitor::assignment_stmt(const AssignmentStmt& stmt) {
  assignment_state.push_assignment_target_rvalue();
  stmt.of_expr->accept_const(*this);
  assignment_state.pop_assignment_target_state();
  auto rhs = pop_type_equation_term();

  value_category_state.push_lhs();
  stmt.to_expr->accept_const(*this);
  value_category_state.pop_side();
  auto lhs = pop_type_equation_term();

  const auto assignment = type_store.make_assignment(lhs.term, rhs.term);
  const auto assignment_var = type_store.make_fresh_type_variable_reference();

  const auto lhs_term = make_term(lhs.source_token, assignment_var);
  const auto rhs_term = make_term(rhs.source_token, assignment);

  push_type_equation(make_eq(lhs_term, rhs_term));
}

}