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
  explicit TypeVisitor(Store& store, TypeStore& type_store, const Library& library,
    const TypeEquality& type_eq, StringRegistry& string_registry) :
    store(store),
    type_store(type_store),
    string_registry(string_registry),
    unifier(type_store, library, string_registry) {
    //
    assignment_state.push_non_assignment_target_rvalue();
    value_category_state.push_rhs();
  }

  ~TypeVisitor() {
    show_type_distribution();
    show_variable_types();
  }

  void show_type_distribution() const;
  void show_variable_types() const;

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
    push_type_equation_term(TypeEquationTerm(expr.source_token, type_store.double_type_handle));
  }

  void char_literal_expr(const CharLiteralExpr& expr) override {
    assert(type_store.char_type_handle.is_valid());
    push_type_equation_term(TypeEquationTerm(expr.source_token, type_store.char_type_handle));
  }

  void string_literal_expr(const StringLiteralExpr& expr) override {
    assert(type_store.string_type_handle.is_valid());
    push_type_equation_term(TypeEquationTerm(expr.source_token, type_store.string_type_handle));
  }

  void literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) override {
    assert(type_store.char_type_handle.is_valid());
    push_type_equation_term(TypeEquationTerm(expr.source_token, type_store.char_type_handle));
  }

  void function_call_expr(const FunctionCallExpr& expr) override;
  void binary_operator_expr(const BinaryOperatorExpr& expr) override;
  void variable_reference_expr(const VariableReferenceExpr& expr) override;
  void function_reference_expr(const FunctionReferenceExpr& expr) override;

  void anonymous_function_expr(const AnonymousFunctionExpr& expr) override {
    assert(false && "Unhandled.");
  }

  void grouping_expr(const GroupingExpr& expr) override;
  void bracket_grouping_expr_lhs(const GroupingExpr& expr);
  void bracket_grouping_expr_rhs(const GroupingExpr& expr);
  void brace_grouping_expr_rhs(const GroupingExpr& expr);

  void expr_stmt(const ExprStmt& stmt) override;
  void assignment_stmt(const AssignmentStmt& stmt) override;

  void function_def_node(const FunctionDefNode& node) override;

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

  void push_type_equation_term(const TypeEquationTerm& term) {
    type_eq_terms.push_back(term);
  }

  TypeEquationTerm pop_type_equation_term() {
    assert(!type_eq_terms.empty());
    const auto term = type_eq_terms.back();
    type_eq_terms.pop_back();
    return term;
  }

private:
  Store& store;
  TypeStore& type_store;
  const StringRegistry& string_registry;

  Unifier unifier;

  AssignmentSourceState assignment_state;
  ValueCategoryState value_category_state;

  std::vector<MatlabScopeHandle> scope_handles;

  std::unordered_map<VariableDefHandle, TypeHandle, VariableDefHandle::Hash> variable_type_handles;
  std::unordered_map<TypeHandle, VariableDefHandle, TypeHandle::Hash> variables;

  std::unordered_map<FunctionDefHandle, TypeHandle, FunctionDefHandle::Hash> function_type_handles;
  std::unordered_map<TypeHandle, FunctionDefHandle, TypeHandle::Hash> functions;

  std::vector<TypeEquationTerm> type_eq_terms;
};

}