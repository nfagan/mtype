#pragma once

#include "type.hpp"
#include "unification.hpp"
#include "type_store.hpp"
#include "library.hpp"
#include "../ast.hpp"
#include "../store.hpp"
#include "../ast/visitor.hpp"
#include "../traversal.hpp"
#include <cassert>
#include <memory>
#include <set>
#include <map>

namespace mt {

class TypeToString;

class TypeConstraintGenerator : public TypePreservingVisitor {
private:
  struct ConstraintRepository {
    std::vector<Type*> variables;
    std::vector<TypeEquation> constraints;
  };

public:
  explicit TypeConstraintGenerator(Substitution& substitution, Store& store, TypeStore& type_store,
                                   Library& library, const StringRegistry& string_registry) :
    substitution(substitution),
    store(store),
    type_store(type_store),
    library(library),
    string_registry(string_registry) {
    //
    assignment_state.push_non_assignment_target_rvalue();
    value_category_state.push_rhs();
    push_monomorphic_functions();
  }

  void show_type_distribution() const;
  void show_variable_types(const TypeToString& printer) const;
  void show_local_function_types(const TypeToString& printer) const;

  void root_block(const RootBlock& block) override;
  void block(const Block& block) override;

  void number_literal_expr(const NumberLiteralExpr& expr) override;
  void char_literal_expr(const CharLiteralExpr& expr) override;
  void string_literal_expr(const StringLiteralExpr& expr) override;

  void dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) override;
  void literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) override;

  void function_call_expr(const FunctionCallExpr& expr) override;
  void function_reference_expr(const FunctionReferenceExpr& expr) override;

  void unary_operator_expr(const UnaryOperatorExpr& expr) override;
  void binary_operator_expr(const BinaryOperatorExpr& expr) override;
  void variable_reference_expr(const VariableReferenceExpr& expr) override;

  void anonymous_function_expr(const AnonymousFunctionExpr& expr) override;
  void superclass_method_reference_expr(const SuperclassMethodReferenceExpr& expr) override;

  void grouping_expr(const GroupingExpr& expr) override;
  void bracket_grouping_expr_lhs(const GroupingExpr& expr);
  void bracket_grouping_expr_rhs(const GroupingExpr& expr);
  void brace_grouping_expr_rhs(const GroupingExpr& expr);
  void parens_grouping_expr_rhs(const GroupingExpr& expr);

  void function_def_node(const FunctionDefNode& node) override;
  void class_def_node(const ClassDefNode& node) override;

  void fun_type_node(const FunTypeNode& node) override;
  void type_annot_macro(const TypeAnnotMacro& macro) override;
  void type_assertion(const TypeAssertion& assertion) override;
  void function_type_node(const FunctionTypeNode& node) override;
  void scalar_type_node(const ScalarTypeNode& node) override;

  void if_stmt(const IfStmt& stmt) override;
  void if_branch(const IfBranch& branch);
  void switch_stmt(const SwitchStmt& stmt) override;
  void try_stmt(const TryStmt& stmt) override;
  void for_stmt(const ForStmt& stmt) override;
  void while_stmt(const WhileStmt& stmt) override;
  void expr_stmt(const ExprStmt& stmt) override;
  void assignment_stmt(const AssignmentStmt& stmt) override;

private:
  std::vector<Type*> grouping_expr_components(const GroupingExpr& expr);

  void gather_function_inputs(const MatlabScope& scope, const FunctionInputParameters& inputs, TypePtrs& into);
  void push_function_inputs(const MatlabScope& scope, const TypePtrs& inputs, const FunctionHeader& header);
  void push_function_outputs(const MatlabScope& scope, const TypePtrs& outputs, const FunctionHeader& header);

  void handle_class_method(const TypePtrs& function_inputs,
                           const TypePtrs& function_outputs,
                           const FunctionAttributes& function_attrs,
                           const MatlabIdentifier& function_name,
                           const Type* function_type_handle,
                           const TypeEquationTerm& rhs_term);

  MT_NODISCARD TypeEquationTerm visit_expr(const BoxedExpr& expr, const Token& source_token);
  Type* make_fresh_type_variable_reference();
  Type* require_bound_type_variable(const VariableDefHandle& variable_def_handle);

  void bind_type_variable_to_variable_def(const VariableDefHandle& def_handle, Type* type_handle);
  void bind_type_variable_to_function_def(const FunctionDefHandle& def_handle, Type* type_handle);
  Type* require_bound_type_variable(const FunctionDefHandle& function_def_handle);

  void push_type_equation(const TypeEquation& eq);

  void push_monomorphic_functions();
  void push_polymorphic_functions();
  void pop_generic_function_state();
  bool are_functions_polymorphic() const;

  void push_constraint_repository();
  ConstraintRepository& current_constraint_repository();
  void pop_constraint_repository();

  void push_type_equation_term(const TypeEquationTerm& term);
  TypeEquationTerm pop_type_equation_term();

private:
  Substitution& substitution;

  Store& store;
  TypeStore& type_store;
  Library& library;
  const StringRegistry& string_registry;

  AssignmentSourceState assignment_state;
  ValueCategoryState value_category_state;
  ClassDefState class_state;

  ScopeState<const MatlabScope> scopes;
  ScopeState<const TypeScope> type_scopes;
  BooleanState polymorphic_function_state;

  std::unordered_map<VariableDefHandle, Type*, VariableDefHandle::Hash> variable_types;
  std::unordered_map<FunctionDefHandle, Type*, FunctionDefHandle::Hash> function_types;

  std::vector<TypeEquationTerm> type_eq_terms;
  std::vector<ConstraintRepository> constraint_repositories;
};

}