#pragma once

#include "mt/mt.hpp"
#include "type.hpp"
#include "unification.hpp"
#include "type_store.hpp"
#include "library.hpp"
#include <cassert>
#include <memory>
#include <set>
#include <map>

namespace mt {

class TypeToString;

class TypeConstraintGenerator : public TypePreservingVisitor {
private:
  struct ConstraintRepository {
    std::vector<TypeHandle> variables;
    std::vector<TypeEquation> constraints;
  };

  struct ScopeStack {
    static void push(TypeConstraintGenerator& vis, const MatlabScopeHandle& handle) {
      vis.push_scope(handle);
    }
    static void pop(TypeConstraintGenerator& vis) {
      vis.pop_scope();
    }
  };
  using MatlabScopeHelper = ScopeHelper<TypeConstraintGenerator, ScopeStack>;

public:
  explicit TypeConstraintGenerator(Substitution& substitution, Store& store, TypeStore& type_store,
                                   const Library& library, StringRegistry& string_registry) :
    substitution(substitution),
    store(store),
    type_store(type_store),
    library(library),
    string_registry(string_registry) {
    //
    assignment_state.push_non_assignment_target_rvalue();
    value_category_state.push_rhs();
  }

  void show_type_distribution() const;
  void show_variable_types(const TypeToString& printer) const;

  void root_block(const RootBlock& block) override;
  void block(const Block& block) override;

  void number_literal_expr(const NumberLiteralExpr& expr) override;
  void char_literal_expr(const CharLiteralExpr& expr) override;
  void string_literal_expr(const StringLiteralExpr& expr) override;

  void dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) override;
  void literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) override;

  void function_call_expr(const FunctionCallExpr& expr) override;
  void binary_operator_expr(const BinaryOperatorExpr& expr) override;
  void variable_reference_expr(const VariableReferenceExpr& expr) override;
  void function_reference_expr(const FunctionReferenceExpr& expr) override;

  void anonymous_function_expr(const AnonymousFunctionExpr& expr) override;

  void grouping_expr(const GroupingExpr& expr) override;
  void bracket_grouping_expr_lhs(const GroupingExpr& expr);
  void bracket_grouping_expr_rhs(const GroupingExpr& expr);
  void brace_grouping_expr_rhs(const GroupingExpr& expr);
  void parens_grouping_expr_rhs(const GroupingExpr& expr);

  void expr_stmt(const ExprStmt& stmt) override;
  void assignment_stmt(const AssignmentStmt& stmt) override;

  void function_def_node(const FunctionDefNode& node) override;

private:
  std::vector<TypeHandle> grouping_expr_components(const GroupingExpr& expr);
  void gather_function_inputs(const MatlabScope& scope, const FunctionInputParameters& inputs, TypeHandles& into);
  TypeEquationTerm visit_expr(const BoxedExpr& expr, const Token& source_token);

  TypeHandle make_fresh_type_variable_reference() {
    auto var_handle = type_store.make_fresh_type_variable_reference();
    if (!constraint_repositories.empty()) {
      constraint_repositories.back().variables.push_back(var_handle);
    }
    return var_handle;
  }

  TypeHandle require_bound_type_variable(const VariableDefHandle& variable_def_handle) {
    if (variable_type_handles.count(variable_def_handle) > 0) {
      return variable_type_handles.at(variable_def_handle);
    }

    const auto variable_type_handle = make_fresh_type_variable_reference();
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

  void push_type_equation(const TypeEquation& eq) {
    if (constraint_repositories.empty()) {
      substitution.push_type_equation(eq);
    } else {
      substitution.push_type_equation(eq);
      constraint_repositories.back().constraints.push_back(eq);
    }
  }

  void push_constraint_repository() {
    constraint_repositories.emplace_back();
  }

  ConstraintRepository& current_constraint_repository() {
    assert(!constraint_repositories.empty());
    return constraint_repositories.back();
  }

  const ConstraintRepository& current_constraint_repository() const {
    assert(!constraint_repositories.empty());
    return constraint_repositories.back();
  }

  void pop_constraint_repository() {
    assert(!constraint_repositories.empty() && "No constraints to pop.");
    constraint_repositories.pop_back();
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
  Substitution& substitution;

  Store& store;
  TypeStore& type_store;
  const Library& library;
  const StringRegistry& string_registry;

  AssignmentSourceState assignment_state;
  ValueCategoryState value_category_state;

  std::vector<MatlabScopeHandle> scope_handles;

  std::unordered_map<VariableDefHandle, TypeHandle, VariableDefHandle::Hash> variable_type_handles;
  std::unordered_map<TypeHandle, VariableDefHandle, TypeHandle::Hash> variables;

  std::unordered_map<FunctionDefHandle, TypeHandle, FunctionDefHandle::Hash> function_type_handles;
  std::unordered_map<TypeHandle, FunctionDefHandle, TypeHandle::Hash> functions;

  std::vector<TypeEquationTerm> type_eq_terms;
  std::vector<ConstraintRepository> constraint_repositories;
};

}