#pragma once

#include "error.hpp"
#include "ast.hpp"
#include "lang_components.hpp"
#include "store.hpp"
#include "Optional.hpp"
#include "traversal.hpp"
#include <unordered_map>
#include <set>
#include <vector>

namespace mt {

class StringRegistry;
class CodeFileDescriptor;

/*
 * VariableAssignmentContext
 */
class VariableAssignmentContext {
public:
  explicit VariableAssignmentContext(int64_t parent_index) : parent_index(parent_index) {
    //
  }
  ~VariableAssignmentContext() = default;
  void register_assignment(const MatlabIdentifier& id, int64_t context_uuid);
  void register_child_context(int64_t context_uuid);

private:
  void register_initialization(const MatlabIdentifier& id, int64_t context_uuid);

public:
  int64_t parent_index;
  std::unordered_map<MatlabIdentifier, std::set<int64_t>, MatlabIdentifier::Hash> context_uuids_by_identifier;
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
    IdentifierContext() : depth(0), scope_depth(0), uuid(0) {
      //
    }
    IdentifierContext(int depth, int scope_depth, int64_t uuid) :
    depth(depth), scope_depth(scope_depth), uuid(uuid) {
      //
    }

    int depth;
    int scope_depth;
    int64_t uuid;
  };

  /*
   * IdentifierInfo
   */
  struct IdentifierInfo {
    IdentifierInfo() : type(IdentifierType::unknown), function_reference() {
      //
    }

    IdentifierInfo(const MatlabIdentifier& source,
                   IdentifierType type,
                   IdentifierContext context,
                   FunctionReferenceHandle reference) :
      source(source), type(type), context(context), function_reference(reference) {
      //
    }

    IdentifierInfo(const MatlabIdentifier& source,
                  IdentifierType type,
                  IdentifierContext context,
                  VariableDefHandle def_handle) :
      source(source), type(type), context(context), variable_def_handle(def_handle) {
      //
    }

    MatlabIdentifier source;
    IdentifierType type;
    IdentifierContext context;

    union {
      FunctionReferenceHandle function_reference;
      VariableDefHandle variable_def_handle;
    };
  };

  /*
   * AssignmentResult
   */
  struct AssignmentResult {
    AssignmentResult(VariableDefHandle resolved_def) :
    success(true), was_initialization(false), variable_def_handle(resolved_def) {

    }
    AssignmentResult(bool success, IdentifierType type, VariableDefHandle new_def) :
    success(success), was_initialization(false), variable_def_handle(new_def), error_already_had_type(type) {
      //
    }

    bool success;
    bool was_initialization;
    VariableDefHandle variable_def_handle;
    IdentifierType error_already_had_type;
  };

  /*
   * ReferenceResult
   */
  struct ReferenceResult {
    ReferenceResult() : success(false) {
      //
    }
    ReferenceResult(bool success, IdentifierInfo info) :
    success(success), info(info) {
      //
    }
    bool success;
    IdentifierInfo info;
  };

public:
  IdentifierScope(IdentifierClassifier* classifier,
                  MatlabScope* parse_scope,
                  int scope_depth,
                  int parent_index) :
    classifier(classifier),
    matlab_scope(parse_scope),
    scope_depth(scope_depth),
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
  AssignmentResult register_variable_assignment(Store::Write& writer,
                                                const MatlabIdentifier& id,
                                                bool force_shadow_parent_assignment = false);
  ReferenceResult register_identifier_reference(Store::Write& read_write,
                                                const MatlabIdentifier& id);

  ReferenceResult register_fully_qualified_import(Store::Write& function_writer,
                                                  const MatlabIdentifier& complete_identifier,
                                                  const MatlabIdentifier& last_identifier_component);

  IdentifierInfo* lookup_variable(const MatlabIdentifier& id, bool traverse_parent);
  FunctionReferenceHandle lookup_local_function(const MatlabIdentifier& name) const;

  bool has_variable(const MatlabIdentifier& id, bool traverse_parent);

  const IdentifierScope* parent() const;
  IdentifierScope* parent();

  void push_context();
  void pop_context();
  void push_variable_assignment_context();
  void pop_variable_assignment_context();

  int current_context_depth() const;
  IdentifierContext current_context() const;
  const IdentifierContext* context_at_depth(int depth) const;

  IdentifierInfo make_local_function_reference_identifier_info(Store::Write& function_read_write,
                                                               const MatlabIdentifier& id,
                                                               FunctionReferenceHandle ref);
  IdentifierInfo make_external_function_reference_identifier_info(Store::Write& function_read_write,
                                                                  const MatlabIdentifier& identifier);
  IdentifierInfo make_function_reference_identifier_info(Store::Write& function_read_write,
                                                         const MatlabIdentifier& identifier,
                                                         FunctionReferenceHandle maybe_local_ref);

private:
  IdentifierClassifier* classifier;
  MatlabScope* matlab_scope;
  int scope_depth;
  int parent_index;

  std::vector<IdentifierContext> contexts;
  int64_t context_uuid;
  std::unordered_map<MatlabIdentifier, IdentifierInfo, MatlabIdentifier::Hash> classified_identifiers;

  std::vector<VariableAssignmentContext> variable_assignment_contexts;
};

/*
 * IdentifierClassifier
 */

class IdentifierClassifier {
  friend class IdentifierScope;

public:
  IdentifierClassifier(StringRegistry* string_registry,
                       Store* store,
                       const CodeFileDescriptor* file_descriptor,
                       std::string_view text);
  ~IdentifierClassifier() = default;

  void transform_root(BoxedRootBlock& block);

  RootBlock* root_block(RootBlock& block);
  Block* block(Block& block);
  FunctionDefNode* function_def_node(FunctionDefNode& def);
  Expr* identifier_reference_expr(IdentifierReferenceExpr& expr);
  Expr* dynamic_field_reference_expr(DynamicFieldReferenceExpr& expr);
  Expr* grouping_expr(GroupingExpr& expr);
  Expr* binary_operator_expr(BinaryOperatorExpr& expr);
  Expr* unary_operator_expr(UnaryOperatorExpr& expr);
  Expr* anonymous_function_expr(AnonymousFunctionExpr& expr);
  Expr* function_reference_expr(FunctionReferenceExpr& expr);
  Expr* presumed_superclass_method_reference_expr(PresumedSuperclassMethodReferenceExpr& expr);
  ClassDefNode* class_def_node(ClassDefNode& ref);

  IfStmt* if_stmt(IfStmt& stmt);
  ExprStmt* expr_stmt(ExprStmt& stmt);
  AssignmentStmt* assignment_stmt(AssignmentStmt& stmt);
  ForStmt* for_stmt(ForStmt& stmt);
  WhileStmt* while_stmt(WhileStmt& stmt);
  SwitchStmt* switch_stmt(SwitchStmt& stmt);
  VariableDeclarationStmt* variable_declaration_stmt(VariableDeclarationStmt& stmt);

  FunTypeNode* fun_type_node(FunTypeNode& node);
  TypeAnnotMacro* type_annot_macro(TypeAnnotMacro& node);

  void if_branch(IfBranch& branch);
  void subscripts(std::vector<Subscript>& subscripts, int64_t begin);
  void subscript(Subscript& sub);

  const ParseErrors& get_errors() const {
    return errors;
  }

  ParseErrors& get_errors() {
    return errors;
  }

  const ParseErrors& get_warnings() const {
    return warnings;
  }

private:
  template <typename T>
  static void conditional_reset(std::unique_ptr<T>& source, T* maybe_new_ptr);

  void push_scope(MatlabScope* parse_scope);
  void pop_scope();

  void push_context();
  void pop_context();
  void register_new_context();

  IdentifierScope* scope_at(int index);
  const IdentifierScope* scope_at(int index) const;
  IdentifierScope* current_scope();
  const IdentifierScope* current_scope() const;

  void register_function_parameter(Store::Write& read_write, const Token& source_token,
    const MatlabIdentifier& identifier);
  void register_function_parameters(Store::Write& read_write, const Token& source_token,
    const std::vector<MatlabIdentifier>& identifiers);
  void register_function_parameters(Store::Write& read_write, const Token& source_token,
    const std::vector<FunctionInputParameter>& identifiers);

  IdentifierScope::AssignmentResult register_variable_assignment(const Token& source_token,
                                                                 const MatlabIdentifier& primary_identifier);
  void register_imports(IdentifierScope* in_scope);
  void register_local_functions(IdentifierScope* in_scope);

  Expr* identifier_reference_expr_lhs(IdentifierReferenceExpr& expr);
  Expr* identifier_reference_expr_rhs(IdentifierReferenceExpr& expr);
  Expr* superclass_method_application(IdentifierReferenceExpr& expr);

  FunctionCallExpr* make_function_call_expr(IdentifierReferenceExpr& from_expr,
                                            int64_t subscript_end,
                                            FunctionReferenceHandle function_reference);

  void block_preserve_context(BoxedBlock& block);
  void block_new_context(BoxedBlock& block);

  void mark_error_identifier(const MatlabIdentifier& identifier);
  bool added_error_for_identifier(const MatlabIdentifier& identifier) const;
  void add_error(ParseError&& error);
  void add_error_if_new_identifier(ParseError&& err, const MatlabIdentifier& id);
  void add_warning_if_new_identifier(ParseError&& err, const MatlabIdentifier& id);

  Optional<ParseError> check_function_reference_subscript(const IdentifierReferenceExpr& expr, int64_t subscript_end);

  ParseError make_error_assignment_to_non_variable(const Token& at_token, const MatlabIdentifier& identifier, IdentifierType present_type);
  ParseError make_error_variable_referenced_before_assignment(const Token& at_token, const MatlabIdentifier& identifier);
  ParseError make_error_implicit_variable_initialization(const Token& at_token);
  ParseError make_error_invalid_function_call_expr(const Token& at_token);
  ParseError make_error_shadowed_import(const Token& at_token, IdentifierType present_type);
  ParseError make_error_pre_declared_qualified_variable(const Token& at_token, const MatlabIdentifier& identifier);
  ParseError make_error_function_reference_to_non_function(const Token& at_token,
                                                           const MatlabIdentifier& identifier,
                                                           IdentifierType present_type);
private:
  StringRegistry* string_registry;
  Store* store;
  const CodeFileDescriptor* file_descriptor;
  std::string_view text;

  std::vector<IdentifierScope> scopes;
  std::vector<std::set<int64_t>> error_identifiers_by_scope;
  int scope_depth;

  ValueCategoryState expr_sides;
  ClassDefState class_state;
  FunctionDefState function_state;

  ParseErrors errors;
  ParseErrors warnings;
};

template <typename T>
void IdentifierClassifier::conditional_reset(std::unique_ptr<T>& source, T* maybe_new_ptr) {
  if (source.get() != maybe_new_ptr) {
    source.reset(maybe_new_ptr);
  }
}

}