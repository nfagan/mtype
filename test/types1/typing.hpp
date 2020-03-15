#pragma once

#include "mt/mt.hpp"
#include "type.hpp"
#include "unification.hpp"
#include "type_store.hpp"
#include <cassert>
#include <memory>
#include <set>
#include <map>

namespace mt {

class TypeVisitor : public TypePreservingVisitor {
  friend class Unifier;
private:
  struct ScopeStack {
    static void push(TypeVisitor& vis, const MatlabScopeHandle& handle) {
      vis.push_scope(handle);
    }
    static void pop(TypeVisitor& vis) {
      vis.pop_scope();
    }
  };
  using MatlabScopeHelper = ScopeHelper<TypeVisitor, ScopeStack>;

public:
  explicit TypeVisitor(Store& store, StringRegistry& string_registry) :
    unifier(type_store, string_registry), store(store), string_registry(string_registry) {
    //  Reserve space
    type_store.reserve(10000);

    type_store.make_builtin_types();
    assignment_state.push_non_assignment_target_rvalue();
    value_category_state.push_rhs();
  }

  ~TypeVisitor() {
//    const auto num_types = double(types.size());
//
//    std::cout << "Num types: " << num_types << std::endl;
//    std::cout << "Size types: " << double(types.size() * sizeof(Type)) / 1024.0 << "kb" << std::endl;
//    std::cout << "Sizeof DebugType: " << sizeof(Type) << std::endl;
//
//    std::unordered_map<Type::Tag, double> counts;
//
//    for (const auto& type : types) {
//      if (counts.count(type.tag) == 0) {
//        counts[type.tag] = 0.0;
//      }
//      counts[type.tag] = counts[type.tag] + 1.0;
//    }
//
//    for (const auto& ct : counts) {
//      std::cout << to_string(ct.first) << ": " << ct.second << " (";
//      std::cout << ct.second/num_types << ")" << std::endl;
//    }
  }

  void root_block(const RootBlock& block) override {
    MatlabScopeHelper scope_helper(*this, block.scope_handle);
    block.block->accept_const(*this);

    unifier.unify();
  }

  void block(const Block& block) override {
    for (const auto& node : block.nodes) {
      node->accept_const(*this);
    }
  }

  void number_literal_expr(const NumberLiteralExpr& expr) override {
    assert(type_store.double_type_handle.is_valid());
    push_type_handle(type_store.double_type_handle);
  }

  void char_literal_expr(const CharLiteralExpr& expr) override {
    assert(type_store.char_type_handle.is_valid());
    push_type_handle(type_store.char_type_handle);
  }

  void string_literal_expr(const StringLiteralExpr& expr) override {
    assert(type_store.string_type_handle.is_valid());
    push_type_handle(type_store.string_type_handle);
  }

  void literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) override {
    assert(type_store.char_type_handle.is_valid());
    push_type_handle(type_store.char_type_handle);
  }

  void function_call_expr(const FunctionCallExpr& expr) override {
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

  void anonymous_function_expr(const AnonymousFunctionExpr& expr) override {
    assert(false && "Unhandled.");
  }

  void variable_reference_expr(const VariableReferenceExpr& expr) override {
    using mt::types::Subscript;
    using mt::types::DestructuredTuple;
    using Use = DestructuredTuple::Usage;

//    assert(expr.subscripts.empty() && "Subscripts not yet handled.");
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

  void binary_operator_expr(const BinaryOperatorExpr& expr) override {
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

  void expr_stmt(const ExprStmt& stmt) override {
    stmt.expr->accept_const(*this);
  }

  void assignment_stmt(const AssignmentStmt& stmt) override {
    /*
     * Grouping expr as assignment -> Tuple of args, with destructuring
     */
    assignment_state.push_assignment_target_rvalue();
    stmt.of_expr->accept_const(*this);
    assignment_state.pop_assignment_target_state();
    auto rhs = pop_type_handle();

    value_category_state.push_lhs();
    stmt.to_expr->accept_const(*this);
    value_category_state.pop_side();
    auto lhs = pop_type_handle();

    unifier.push_type_equation(TypeEquation(lhs, rhs));
  }

  void grouping_expr(const GroupingExpr& expr) override {
    if (expr.method == GroupingMethod::brace) {
      assert(!value_category_state.is_lhs());
      brace_grouping_expr_rhs(expr);

    } else if (expr.method == GroupingMethod::bracket && value_category_state.is_lhs()) {
      bracket_grouping_expr_lhs(expr);

    } else {
      assert(false && "Unhandled grouping expr.");
    }
  }

  void bracket_grouping_expr_lhs(const GroupingExpr& expr) {
    using types::DestructuredTuple;
    using Use = DestructuredTuple::Usage;

    std::vector<TypeHandle> members;
    members.reserve(expr.components.size());

    for (const auto& component : expr.components) {
      auto tvar = type_store.make_fresh_type_variable_reference();
      push_type_handle(tvar);
      component.expr->accept_const(*this);
      auto res = pop_type_handle();

      unifier.push_type_equation(TypeEquation(tvar, res));
      members.push_back(res);
    }

    auto tup_type = type_store.make_type();
    type_store.assign(tup_type, Type(DestructuredTuple(Use::lvalue, std::move(members))));
    push_type_handle(tup_type);
  }

  void brace_grouping_expr_rhs(const GroupingExpr& expr) {
    using types::List;
    using types::Tuple;

    List list;
    list.pattern.reserve(expr.components.size());

    for (const auto& component : expr.components) {
      auto tvar = type_store.make_fresh_type_variable_reference();
      push_type_handle(tvar);
      component.expr->accept_const(*this);
      auto res = pop_type_handle();

      unifier.push_type_equation(TypeEquation(tvar, res));
      list.pattern.push_back(res);
    }

    auto list_handle = type_store.make_type();
    auto tuple_handle = type_store.make_type();

    type_store.assign(list_handle, Type(std::move(list)));
    type_store.assign(tuple_handle, Type(Tuple(list_handle)));

    push_type_handle(tuple_handle);
  }

  void function_def_node(const FunctionDefNode& node) override {
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

  const Type& at(const TypeHandle& handle) const {
    return type_store.at(handle);
  }

  Type& at(const TypeHandle& handle) {
    return type_store.at(handle);
  }

private:
  bool is_bound_type_variable(const VariableDefHandle& handle) {
    return variable_type_handles.count(handle) > 0;
  }

  TypeHandle require_bound_type_variable(const VariableDefHandle& variable_def_handle) {
    if (is_bound_type_variable(variable_def_handle)) {
      return variable_type_handles.at(variable_def_handle);
    }

    const auto variable_type_handle = type_store.make_type();
    type_store.assign(variable_type_handle, Type(type_store.make_type_variable()));
    bind_type_variable_to_variable_def(variable_def_handle, variable_type_handle);

    return variable_type_handle;
  }

  void bind_type_variable_to_variable_def(const VariableDefHandle& def_handle, const TypeHandle& type_handle) {
    variable_type_handles[def_handle] = type_handle;
    variables[type_handle] = def_handle;
  }

  void bind_type_variable_to_function_def(const FunctionDefHandle& def_handle, const TypeHandle& type_handle) {
    function_type_handles[def_handle] = type_handle;
    functions[type_handle] = def_handle;
  }

  void push_scope(const MatlabScopeHandle& handle) {
    scope_handles.push_back(handle);
  }
  void pop_scope() {
    scope_handles.pop_back();
  }
  MatlabScopeHandle current_scope_handle() const {
    return scope_handles.back();
  }

  void push_type_handle(const TypeHandle& handle) {
    type_handles.push_back(handle);
  }

  TypeHandle pop_type_handle() {
    assert(!type_handles.empty());
    const auto handle = type_handles.back();
    type_handles.pop_back();
    return handle;
  }
public:
  Unifier unifier;

private:
  Store& store;
  TypeStore type_store;
  const StringRegistry& string_registry;

  AssignmentSourceState assignment_state;
  ValueCategoryState value_category_state;

  std::vector<MatlabScopeHandle> scope_handles;

  std::unordered_map<VariableDefHandle, TypeHandle, VariableDefHandle::Hash> variable_type_handles;
  std::unordered_map<TypeHandle, VariableDefHandle, TypeHandle::Hash> variables;

  std::unordered_map<FunctionDefHandle, TypeHandle, FunctionDefHandle::Hash> function_type_handles;
  std::unordered_map<TypeHandle, FunctionDefHandle, TypeHandle::Hash> functions;

  std::vector<TypeHandle> type_handles;
};

}