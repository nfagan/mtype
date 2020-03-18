#include "typing.hpp"

namespace mt {

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
        function_inputs.emplace_back();
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

void TypeVisitor::function_call_expr(const FunctionCallExpr& expr) {
  using types::DestructuredTuple;
  using types::Abstraction;
  using Use = DestructuredTuple::Usage;

  std::vector<TypeHandle> members;
  members.reserve(expr.arguments.size());

  for (const auto& arg : expr.arguments) {
    auto tvar = type_store.make_fresh_type_variable_reference();
    push_type_handle(tvar);
    arg->accept_const(*this);
    auto result_var = pop_type_handle();

    unifier.push_type_equation(TypeEquation(tvar, result_var));
    members.push_back(result_var);
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

  push_type_handle(result_type);
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
    push_type_handle(tup_handle);

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
        push_type_handle(var_handle);
        arg_expr->accept_const(*this);
        auto res_handle = pop_type_handle();
        args.push_back(res_handle);

        unifier.push_type_equation(TypeEquation(var_handle, res_handle));
      }

      subscripts.emplace_back(Subscript::Sub(method, std::move(args)));
    }

    value_category_state.pop_side();

    auto sub_type = type_store.make_type();
    auto outputs_type = type_store.make_fresh_type_variable_reference();

    type_store.assign(sub_type, Type(Subscript(variable_type_handle, std::move(subscripts), outputs_type)));
    unifier.push_type_equation(TypeEquation(type_store.make_fresh_type_variable_reference(), sub_type));

    push_type_handle(outputs_type);
  }
}

void TypeVisitor::binary_operator_expr(const BinaryOperatorExpr& expr) {
  using namespace mt::types;

  auto tvar_lhs = type_store.make_fresh_type_variable_reference();
  push_type_handle(tvar_lhs);
  expr.left->accept_const(*this);
  auto lhs_result = pop_type_handle();

  if (variables.count(lhs_result) == 0) {
    unifier.push_type_equation(TypeEquation(tvar_lhs, lhs_result));
  }

  auto tvar_rhs = type_store.make_fresh_type_variable_reference();
  push_type_handle(tvar_rhs);
  expr.right->accept_const(*this);
  auto rhs_result = pop_type_handle();

  if (variables.count(rhs_result) == 0) {
    unifier.push_type_equation(TypeEquation(tvar_rhs, rhs_result));
  }

  const auto output_handle = type_store.make_fresh_type_variable_reference();
  const auto inputs_handle = type_store.make_type();

  type_store.assign(inputs_handle,
    Type(DestructuredTuple(DestructuredTuple::Usage::rvalue, lhs_result, rhs_result)));

  types::Abstraction func_type(expr.op, inputs_handle, output_handle);
  const auto func_handle = type_store.make_type();
  type_store.assign(func_handle, Type(std::move(func_type)));
  unifier.push_type_equation(TypeEquation(type_store.make_fresh_type_variable_reference(), func_handle));

  push_type_handle(output_handle);
}

}