#pragma once

#include "error.hpp"
#include "ast.hpp"
#include "lang_components.hpp"
#include "FunctionRegistry.hpp"
#include "Optional.hpp"
#include <unordered_map>
#include <set>
#include <vector>

namespace mt {

class StringRegistry;

/*
 * VariableAssignmentContext
 */
class VariableAssignmentContext {
public:
  explicit VariableAssignmentContext(int64_t parent_index) : parent_index(parent_index) {
    //
  }
  ~VariableAssignmentContext() = default;
  void register_assignment(int64_t id, int64_t context_uuid);
  void register_child_context(int64_t context_uuid);

private:
  void register_initialization(int64_t id, int64_t context_uuid);

public:
  int64_t parent_index;
  std::unordered_map<int64_t, std::set<int64_t>> context_uuids_by_identifier;
  std::vector<int64_t> child_context_uuids;
};

/*
 * IdentifierScope
 */
class IdentifierScope {
  friend class IdentifierClassifier;

  /*
   * IdentifierContext
   */
  struct IdentifierContext {
    IdentifierContext() : depth(0), uuid(0) {
      //
    }
    IdentifierContext(int depth, int64_t uuid) : depth(depth), uuid(uuid) {
      //
    }

    int depth;
    int64_t uuid;
  };

  /*
   * IdentifierInfo
   */
  struct IdentifierInfo {
    IdentifierInfo() : type(IdentifierType::unknown), function_reference(nullptr) {
      //
    }

    IdentifierInfo(IdentifierType type, IdentifierContext context) :
      IdentifierInfo(type, context, static_cast<FunctionReference*>(nullptr)) {
      //
    }

    IdentifierInfo(IdentifierType type, IdentifierContext context, FunctionReference* reference) :
      type(type), context(context), function_reference(reference) {
      //
    }

    IdentifierInfo(IdentifierType type, IdentifierContext context, VariableDef* definition) :
      type(type), context(context), variable_def(definition) {
      //
    }

    IdentifierType type;
    IdentifierContext context;

    union {
      FunctionReference* function_reference;
      VariableDef* variable_def;
    };
  };

  /*
   * AssignmentResult
   */
  struct AssignmentResult {
    AssignmentResult(VariableDef* resolved_def) :
    success(true), was_initialization(false), variable_def(resolved_def) {

    }
    AssignmentResult(bool success, IdentifierType type, VariableDef* new_def) :
    success(success), was_initialization(false), variable_def(new_def), error_already_had_type(type) {
      //
    }

    bool success;
    bool was_initialization;
    VariableDef* variable_def;
    IdentifierType error_already_had_type;
  };

  /*
   * ReferenceResult
   */
  struct ReferenceResult {
    ReferenceResult(bool success, IdentifierInfo info) :
    success(success), info(info) {
      //
    }
    bool success;
    IdentifierInfo info;
  };

public:
  IdentifierScope(IdentifierClassifier* classifier, std::shared_ptr<MatlabScope> parse_scope, int parent_index) :
    classifier(classifier),
    matlab_scope(std::move(parse_scope)),
    parent_index(parent_index),
    context_uuid(0) {
    push_context();
  }

#ifndef _MSC_VER
  IdentifierScope(IdentifierScope&& other) noexcept = default;
  IdentifierScope& operator=(IdentifierScope&& other) noexcept = default;
#endif
  ~IdentifierScope() = default;

private:
  bool has_parent() const;
  AssignmentResult register_variable_assignment(int64_t id, bool force_shadow_parent_assignment = false);
  ReferenceResult register_identifier_reference(int64_t id);
  ReferenceResult register_external_function_reference(int64_t id);
  ReferenceResult register_fully_qualified_import(int64_t complete_identifier, int64_t last_identifier_component);

  IdentifierInfo* lookup_variable(int64_t id, bool traverse_parent);
  FunctionReference* lookup_local_function(int64_t name) const;
  static FunctionReference* lookup_local_function(int64_t name, const std::shared_ptr<MatlabScope>& lookup_scope);

  bool has_variable(int64_t id, bool traverse_parent);

  const IdentifierScope* parent() const;
  IdentifierScope* parent();

  void push_context();
  void pop_context();
  void push_variable_assignment_context();
  void pop_variable_assignment_context();

  int current_context_depth() const;
  IdentifierContext current_context() const;
  const IdentifierContext* context_at_depth(int depth) const;

  IdentifierInfo make_local_function_reference_identifier_info(FunctionReference* ref);
  IdentifierInfo make_external_function_reference_identifier_info(int64_t identifier);
  IdentifierInfo make_function_reference_identifier_info(int64_t identifier, FunctionReference* maybe_local_ref);

private:
  IdentifierClassifier* classifier;
  std::shared_ptr<MatlabScope> matlab_scope;
  int parent_index;

  std::vector<IdentifierContext> contexts;
  int64_t context_uuid;
  std::unordered_map<int64_t, IdentifierInfo> classified_identifiers;

  std::vector<VariableAssignmentContext> variable_assignment_contexts;
};

/*
 * IdentifierClassifier
 */

class IdentifierClassifier {
  friend class IdentifierScope;

public:
  IdentifierClassifier(StringRegistry* string_registry,
                       FunctionRegistry* function_registry, std::string_view text);
  ~IdentifierClassifier() = default;

  RootBlock* root_block(RootBlock& block);
  Block* block(Block& block);
  FunctionReference* function_reference(FunctionReference& ref);
  Expr* identifier_reference_expr(IdentifierReferenceExpr& expr);
  Expr* dynamic_field_reference_expr(DynamicFieldReferenceExpr& expr);
  Expr* grouping_expr(GroupingExpr& expr);
  Expr* binary_operator_expr(BinaryOperatorExpr& expr);
  Expr* unary_operator_expr(UnaryOperatorExpr& expr);

  IfStmt* if_stmt(IfStmt& stmt);
  ExprStmt* expr_stmt(ExprStmt& stmt);
  AssignmentStmt* assignment_stmt(AssignmentStmt& stmt);

  void if_branch(IfBranch& branch);
  void subscripts(std::vector<Subscript>& subscripts, int64_t begin);
  void subscript(Subscript& sub);

  const ParseErrors& get_errors() const {
    return errors;
  }

private:
  template <typename T>
  static void conditional_reset(std::unique_ptr<T>& source, T* maybe_new_ptr);

  void push_scope(const std::shared_ptr<MatlabScope>& parse_scope);
  void pop_scope();

  void push_context();
  void pop_context();
  void register_new_context();

  void push_lhs();
  void push_rhs();
  void pop_expr_side();

  bool is_lhs() const;

  IdentifierScope* scope_at(int index);
  const IdentifierScope* scope_at(int index) const;
  IdentifierScope* current_scope();
  const IdentifierScope* current_scope() const;

  void register_function_parameters(const Token& source_token, std::vector<int64_t>& identifiers);
  IdentifierScope::AssignmentResult register_variable_assignment(const Token& source_token, int64_t primary_identifier);
  void register_imports(IdentifierScope* in_scope);

  Expr* identifier_reference_expr_lhs(IdentifierReferenceExpr& expr);
  Expr* identifier_reference_expr_rhs(IdentifierReferenceExpr& expr);

  FunctionCallExpr* make_function_call_expr(IdentifierReferenceExpr& from_expr,
                                            int64_t subscript_end,
                                            FunctionReference* function_reference);

  void block_preserve_context(BoxedBlock& block);
  void block_new_context(BoxedBlock& block);

  void mark_error_identifier(int64_t identifier);
  bool added_error_for_identifier(int64_t identifier) const;
  void add_error(ParseError&& error);
  void add_error_if_new_identifier(ParseError&& err, int64_t identifier);

  Optional<ParseError> check_function_reference_subscript(const IdentifierReferenceExpr& expr, int64_t subscript_end);

  ParseError make_error_assignment_to_non_variable(const Token& at_token, int64_t identifier, IdentifierType present_type);
  ParseError make_error_variable_referenced_before_assignment(const Token& at_token, int64_t identifier);
  ParseError make_error_implicit_variable_initialization(const Token& at_token);
  ParseError make_error_invalid_function_call_expr(const Token& at_token);
  ParseError make_error_invalid_function_index_expr(const Token& at_token);
  ParseError make_error_shadowed_import(const Token& at_token, IdentifierType present_type);

private:
  StringRegistry* string_registry;
  FunctionRegistry* function_registry;
  std::string_view text;

  std::vector<IdentifierScope> scopes;
  std::vector<std::set<int64_t>> error_identifiers_by_scope;
  int scope_depth;

  std::vector<bool> expr_sides;

  ParseErrors errors;
};

template <typename T>
void IdentifierClassifier::conditional_reset(std::unique_ptr<T>& source, T* maybe_new_ptr) {
  if (source.get() != maybe_new_ptr) {
    source.reset(maybe_new_ptr);
  }
}

}