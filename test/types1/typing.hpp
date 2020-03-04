#pragma once

#include "mt/mt.hpp"
#include "type.hpp"
#include <cassert>
#include <memory>
#include <set>
#include <map>

namespace mt {

class TypeVisitor;

struct TypeEquation {
  TypeEquation(const TypeHandle& lhs, const TypeHandle& rhs) : lhs(lhs), rhs(rhs) {
    //
  }

  TypeHandle lhs;
  TypeHandle rhs;
};

class Substitution : public TypeExprVisitor {
public:
  using BoundVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;
public:
  explicit Substitution(TypeVisitor& visitor) : visitor(visitor) {
    //
  }

  ~Substitution() {
    std::cout << "Num type eqs: " << type_equations.size() << std::endl;
    std::cout << "Subs size: " << substitution.size() << std::endl;
  }

  void push_type_equation(TypeEquation&& eq) {
    type_equations.emplace_back(std::move(eq));
  }

  void unify();

private:
  void unify_one(TypeEquation eq);
  void substitute_function(types::Function& func);
  void substitute_variable(TypeHandle& var_handle, types::Variable& var);
  void substitute_dispatch(TypeHandle& handle);
  DebugType::Tag type_of(const TypeHandle& handle) const;
  void show();

private:
  TypeVisitor& visitor;

  std::vector<TypeEquation> type_equations;
  BoundVariables bound_variables;
  std::vector<TypeEquation> substitution;
};

class TypeVisitor : public TypePreservingVisitor {
  friend class Substitution;
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
  explicit TypeVisitor(Store& store, const StringRegistry& string_registry) :
  substitution(*this), store(store), string_registry(string_registry), type_variable_ids(0) {
    make_builtin_types();
  }

  void make_builtin_types() {
    double_type_handle = make_type();
    string_type_handle = make_type();
    char_type_handle = make_type();

    assign(double_type_handle, DebugType(types::Scalar(make_type_identifier())));
    assign(string_type_handle, DebugType(types::Scalar(make_type_identifier())));
    assign(char_type_handle, DebugType(types::Scalar(make_type_identifier())));
  }

  void root_block(const RootBlock& block) override {
    MatlabScopeHelper scope_helper(*this, block.scope_handle);
    block.block->accept_const(*this);

    substitution.unify();
  }

  void block(const Block& block) override {
    for (const auto& node : block.nodes) {
      node->accept_const(*this);
    }
  }

  void number_literal_expr(const NumberLiteralExpr& expr) override {
    assert(double_type_handle.is_valid());
    push_type_handle(double_type_handle);
  }

  void char_literal_expr(const CharLiteralExpr& expr) override {
    assert(char_type_handle.is_valid());
    push_type_handle(char_type_handle);
  }

  void string_literal_expr(const StringLiteralExpr& expr) override {
    assert(string_type_handle.is_valid());
    push_type_handle(string_type_handle);
  }

  void function_call_expr(const FunctionCallExpr& expr) override {
    assert(false && "Unhandled");
  }

  void anonymous_function_expr(const AnonymousFunctionExpr& expr) override {
    assert(false && "Unhandled.");
  }

  void variable_reference_expr(const VariableReferenceExpr& expr) override {
    assert(expr.subscripts.empty() && "Subscripts not yet handled.");
    const auto variable_type_handle = require_bound_type_variable(expr.def_handle);
    push_type_handle(variable_type_handle);
  }

  void binary_operator_expr(const BinaryOperatorExpr& expr) override {
    auto tvar_lhs = make_fresh_type_variable_reference();
    push_type_handle(tvar_lhs);
    expr.left->accept_const(*this);
    auto lhs_result = pop_type_handle();

    if (variables.count(lhs_result) == 0) {
      substitution.push_type_equation(TypeEquation(tvar_lhs, lhs_result));
    }

    auto tvar_rhs = make_fresh_type_variable_reference();
    push_type_handle(tvar_rhs);
    expr.right->accept_const(*this);
    auto rhs_result = pop_type_handle();

    if (variables.count(rhs_result) == 0) {
      substitution.push_type_equation(TypeEquation(tvar_rhs, rhs_result));
    }

    std::vector<TypeHandle> arguments;
    arguments.reserve(2);
    arguments.push_back(lhs_result);
    arguments.push_back(rhs_result);

    texpr::Application app(expr.op, std::move(arguments), TypeHandle());
    auto app_it = applications.find(app);
    TypeHandle result_handle;

    if (app_it == applications.end()) {
      const auto output_handle = make_fresh_type_variable_reference();
      app.outputs[0] = output_handle;

      result_handle = make_type();
      types::Function func_type;
      func_type.inputs = app.arguments;
      func_type.outputs = app.outputs;
      assign(result_handle, DebugType(std::move(func_type)));
      applications.emplace(app, output_handle);

      const auto tvar = make_fresh_type_variable_reference();
      substitution.push_type_equation(TypeEquation(tvar, result_handle));

      result_handle = output_handle;

    } else {
      result_handle = app_it->second;
    }

    push_type_handle(result_handle);
  }

  void expr_stmt(const ExprStmt& stmt) override {
    stmt.expr->accept_const(*this);
  }

  void assignment_stmt(const AssignmentStmt& stmt) override {
    stmt.of_expr->accept_const(*this);
    auto rhs = pop_type_handle();

    stmt.to_expr->accept_const(*this);
    auto lhs = pop_type_handle();

    substitution.push_type_equation(TypeEquation(lhs, rhs));
  }

  void function_def_node(const FunctionDefNode& node) override {
    const Block* function_body = nullptr;
    MatlabScopeHelper scope_helper(*this, node.scope_handle);

    const auto type_handle = make_type();
    bind_type_variable_to_function_def(node.def_handle, type_handle);

    types::Function function_type;

    {
      const auto scope_handle = current_scope_handle();

      Store::ReadConst reader(store);
      const auto& def = reader.at(node.def_handle);
      const auto& scope = reader.at(scope_handle);

      for (const auto& input : def.header.inputs) {
        if (!input.is_ignored) {
          assert(input.name.full_name() >= 0);

          const auto variable_def_handle = scope.local_variables.at(input.name);
          const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

          function_type.inputs.push_back(variable_type_handle);
        } else {
          function_type.inputs.emplace_back();
        }
      }

      for (const auto& output : def.header.outputs) {
        assert(output.full_name() >= 0);

        const auto variable_def_handle = scope.local_variables.at(output);
        const auto variable_type_handle = require_bound_type_variable(variable_def_handle);

        function_type.outputs.push_back(variable_type_handle);
      }

      function_body = def.body.get();
    }

    assign(type_handle, DebugType(std::move(function_type)));
    substitution.push_type_equation(TypeEquation(make_fresh_type_variable_reference(), type_handle));

    if (function_body) {
      function_body->accept_const(*this);
    }
  }

  const DebugType& at(const TypeHandle& handle) const {
    assert(handle.is_valid() && handle.get_index() < types.size());
    return types[handle.get_index()];
  }

  DebugType& at(const TypeHandle& handle) {
    assert(handle.is_valid() && handle.get_index() < types.size());
    return types[handle.get_index()];
  }

private:
  bool is_bound_type_variable(const VariableDefHandle& handle) {
    return variable_type_handles.count(handle) > 0;
  }

  TypeHandle require_bound_type_variable(const VariableDefHandle& variable_def_handle) {
    if (is_bound_type_variable(variable_def_handle)) {
      return variable_type_handles.at(variable_def_handle);
    }

    const auto variable_type_handle = make_type();
    assign(variable_type_handle, DebugType(make_type_variable()));
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

  TypeHandle make_type() {
    types.emplace_back();
    return TypeHandle(types.size() - 1);
  }

  TypeHandle make_fresh_type_variable_reference() {
    auto handle = make_type();
    assign(handle, DebugType(make_type_variable()));
    return handle;
  }

  types::Variable make_type_variable() {
    return types::Variable(make_type_identifier());
  }

  TypeIdentifier make_type_identifier() {
    return TypeIdentifier(type_variable_ids++);
  }

  void assign(const TypeHandle& at, DebugType&& type) {
    assert(at.is_valid() && at.get_index() < types.size());
    types[at.get_index()] = std::move(type);
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
  Substitution substitution;

private:
  Store& store;
  const StringRegistry& string_registry;

  std::vector<MatlabScopeHandle> scope_handles;
  int64_t type_variable_ids;

  std::unordered_map<VariableDefHandle, TypeHandle, VariableDefHandle::Hash> variable_type_handles;
  std::unordered_map<TypeHandle, VariableDefHandle, TypeHandle::Hash> variables;

  std::unordered_map<FunctionDefHandle, TypeHandle, FunctionDefHandle::Hash> function_type_handles;
  std::unordered_map<TypeHandle, FunctionDefHandle, TypeHandle::Hash> functions;
  std::vector<DebugType> types;

  std::vector<TypeHandle> type_handles;

  TypeHandle double_type_handle;
  TypeHandle string_type_handle;
  TypeHandle char_type_handle;

  std::map<texpr::Application, TypeHandle, texpr::Application::Less> applications;
};

}