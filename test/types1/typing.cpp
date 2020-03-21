#include "typing.hpp"
#include "debug.hpp"

namespace mt {

void TypeVisitor::show_type_distribution() const {
  auto counts = type_store.type_distribution();
  const auto num_types = double(counts.size());

  for (const auto& ct : counts) {
    std::cout << to_string(ct.first) << ": " << ct.second << " (";
    std::cout << ct.second/num_types << ")" << std::endl;
  }
}

void TypeVisitor::show_variable_types() const {
  std::cout << "--" << std::endl;

  Store::ReadConst reader(store);

  for (const auto& var_it : variable_type_handles) {
    const auto& maybe_type = unifier.bound_type(var_it.second);
    const Type& type = maybe_type ? type_store.at(maybe_type.value()) : type_store.at(var_it.second);

    const auto& def_handle = var_it.first;
    const auto& def = reader.at(def_handle);
    const auto& name = string_registry.at(def.name.full_name());

    std::cout << name << ": ";
    DebugTypePrinter(type_store, &string_registry).show(type);
    std::cout << std::endl;
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

    for (const auto& input : def.header.inputs) {
      if (!input.is_ignored) {
        assert(input.name.full_name() >= 0);

        const auto variable_def_handle = scope.local_variables.at(input.name);
        const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

        function_inputs.push_back(variable_type_handle);
      } else {
        //  @TODO: Change this type to Top
        function_inputs.push_back(type_store.make_fresh_type_variable_reference());
      }
    }

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

  type_store.assign(type_handle, Type(Abstraction(function_name, input_handle, output_handle)));
  unifier.push_type_equation(TypeEquation(type_store.make_fresh_type_variable_reference(), type_handle));

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
    auto tvar = type_store.make_fresh_type_variable_reference();
    push_type_equation_term(TypeEquationTerm(expr.source_token, tvar));
    arg->accept_const(*this);
    auto result = pop_type_equation_term();

    unifier.push_type_equation(TypeEquation(tvar, result.term));
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

  type_store.assign(func_type, Type(Abstraction(function_name, args_type, result_type)));
  unifier.push_type_equation(TypeEquation(type_store.make_fresh_type_variable_reference(), func_type));

  push_type_equation_term(TypeEquationTerm(expr.source_token, result_type));
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
    push_type_equation_term(TypeEquationTerm(expr.source_token, tup_handle));

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
        push_type_equation_term(TypeEquationTerm(expr.source_token, var_handle));
        arg_expr->accept_const(*this);
        auto res = pop_type_equation_term();
        args.push_back(res.term);

        unifier.push_type_equation(TypeEquation(var_handle, res.term));
      }

      subscripts.emplace_back(Subscript::Sub(method, std::move(args)));
    }

    value_category_state.pop_side();

    auto sub_type = type_store.make_type();
    auto outputs_type = type_store.make_fresh_type_variable_reference();

    type_store.assign(sub_type, Type(Subscript(variable_type_handle, std::move(subscripts), outputs_type)));
    unifier.push_type_equation(TypeEquation(type_store.make_fresh_type_variable_reference(), sub_type));

    push_type_equation_term(TypeEquationTerm(expr.source_token, outputs_type));
  }
}

void TypeVisitor::function_reference_expr(const FunctionReferenceExpr& expr) {
  Store::ReadConst reader(store);
  const auto& ref = reader.at(expr.handle);
  assert(!ref.def_handle.is_valid() && "Local funcs not yet handled.");

  auto inputs = type_store.make_fresh_type_variable_reference();
  auto outputs = type_store.make_fresh_type_variable_reference();
  auto func = type_store.make_abstraction(ref.name, inputs, outputs);

  push_type_equation_term(TypeEquationTerm(expr.source_token, func));
}

void TypeVisitor::binary_operator_expr(const BinaryOperatorExpr& expr) {
  using namespace mt::types;

  auto tvar_lhs = type_store.make_fresh_type_variable_reference();
  push_type_equation_term(TypeEquationTerm(expr.source_token, tvar_lhs));
  expr.left->accept_const(*this);
  auto lhs_result = pop_type_equation_term();

  if (variables.count(lhs_result.term) == 0) {
    unifier.push_type_equation(TypeEquation(tvar_lhs, lhs_result.term));
  }

  auto tvar_rhs = type_store.make_fresh_type_variable_reference();
  push_type_equation_term(TypeEquationTerm(expr.source_token, tvar_rhs));
  expr.right->accept_const(*this);
  auto rhs_result = pop_type_equation_term();

  if (variables.count(rhs_result.term) == 0) {
    unifier.push_type_equation(TypeEquation(tvar_rhs, rhs_result.term));
  }

  const auto output_handle = type_store.make_fresh_type_variable_reference();
  const auto inputs_handle = type_store.make_type();

  type_store.assign(inputs_handle,
    Type(DestructuredTuple(DestructuredTuple::Usage::rvalue, lhs_result.term, rhs_result.term)));

  types::Abstraction func_type(expr.op, inputs_handle, output_handle);
  const auto func_handle = type_store.make_type();
  type_store.assign(func_handle, Type(std::move(func_type)));
  unifier.push_type_equation(TypeEquation(type_store.make_fresh_type_variable_reference(), func_handle));

  push_type_equation_term(TypeEquationTerm(expr.source_token, output_handle));
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

  } else {
    assert(false && "Unhandled grouping expr.");
  }
}

void TypeVisitor::bracket_grouping_expr_lhs(const GroupingExpr& expr) {
  using types::DestructuredTuple;
  using Use = DestructuredTuple::Usage;

  std::vector<TypeHandle> members;
  members.reserve(expr.components.size());

  for (const auto& component : expr.components) {
    auto tvar = type_store.make_fresh_type_variable_reference();
    push_type_equation_term(TypeEquationTerm(expr.source_token, tvar));
    component.expr->accept_const(*this);
    auto res = pop_type_equation_term();

    unifier.push_type_equation(TypeEquation(tvar, res.term));
    members.push_back(res.term);
  }

  auto tup_type = type_store.make_type();
  type_store.assign(tup_type, Type(DestructuredTuple(Use::lvalue, std::move(members))));
  push_type_equation_term(TypeEquationTerm(expr.source_token, tup_type));
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
    push_type_equation_term(TypeEquationTerm(expr.source_token, tvar));
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

  unifier.push_type_equation(TypeEquation(type_store.make_fresh_type_variable_reference(), abstr));
  push_type_equation_term(TypeEquationTerm(expr.source_token, result_handle));
}

void TypeVisitor::brace_grouping_expr_rhs(const GroupingExpr& expr) {
  using types::List;
  using types::Tuple;

  List list;
  list.pattern.reserve(expr.components.size());

  for (const auto& component : expr.components) {
    auto tvar = type_store.make_fresh_type_variable_reference();
    push_type_equation_term(TypeEquationTerm(expr.source_token, tvar));
    component.expr->accept_const(*this);
    auto res = pop_type_equation_term();

    unifier.push_type_equation(TypeEquation(tvar, res.term));
    list.pattern.push_back(res.term);
  }

  auto list_handle = type_store.make_type();
  auto tuple_handle = type_store.make_type();

  type_store.assign(list_handle, Type(std::move(list)));
  type_store.assign(tuple_handle, Type(Tuple(list_handle)));

  push_type_equation_term(TypeEquationTerm(expr.source_token, tuple_handle));
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
  unifier.push_type_equation(TypeEquation(assignment_var, assignment));
}

}