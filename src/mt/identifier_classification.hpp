#pragma once

#include "error.hpp"
#include "ast.hpp"
#include "lang_components.hpp"
#include "Optional.hpp"
#include <unordered_map>
#include <set>
#include <vector>

namespace mt {

struct ParseScope;
class StringRegistry;

class IdentifierScope {
  friend class IdentifierClassifier;

  struct AssignmentResult {
    AssignmentResult() : success(true) {

    }
    AssignmentResult(bool success, IdentifierType type) : success(success), error_already_had_type(type) {
      //
    }

    bool success;
    IdentifierType error_already_had_type;
  };

  struct ReferenceResult {
    ReferenceResult(bool success, IdentifierType classified_type) :
    success(success), classified_type(classified_type) {
      //
    }
    bool success;
    IdentifierType classified_type;
  };

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

  struct IdentifierInfo {
    IdentifierInfo() = default;

    IdentifierInfo(IdentifierType type, IdentifierContext context) :
    IdentifierInfo(type, context, nullptr) {
      //
    }

    IdentifierInfo(IdentifierType type, IdentifierContext context, FunctionDef* definition) :
      type(type), context(context), definition(definition) {
      //
    }

    IdentifierType type;
    IdentifierContext context;
    FunctionDef* definition;
  };

public:
  IdentifierScope(IdentifierClassifier* classifier, std::shared_ptr<ParseScope> parse_scope, int parent_index) :
  classifier(classifier),
  parse_scope(std::move(parse_scope)),
  parent_index(parent_index),
  context_uuid(0) {
    push_context();
  }
  IdentifierScope(IdentifierScope&& other) noexcept = default;
  IdentifierScope& operator=(IdentifierScope&& other) noexcept = default;
  ~IdentifierScope() = default;

private:
  bool has_parent() const;
  AssignmentResult register_variable_assignment(int64_t id);
  ReferenceResult register_identifier_reference(int64_t id);
  IdentifierInfo* lookup_variable(int64_t id, bool traverse_parent);
  FunctionDef* lookup_function(int64_t name) const;

  static FunctionDef* lookup_function(int64_t name, const std::shared_ptr<ParseScope>& lookup_scope);

  const IdentifierScope* parent() const;
  IdentifierScope* parent();

  void push_context();
  void pop_context();
  int current_context_depth() const;
  IdentifierContext current_context() const;
  const IdentifierContext* context_at_depth(int depth) const;

private:
  IdentifierClassifier* classifier;
  std::shared_ptr<ParseScope> parse_scope;
  int parent_index;
  std::set<int64_t> sibling_functions;
  std::set<int64_t> child_functions;

  std::vector<IdentifierContext> contexts;
  int64_t context_uuid;
  std::unordered_map<int64_t, IdentifierInfo> classified_identifiers;
};

class IdentifierClassifier {
  friend class IdentifierScope;

public:
  IdentifierClassifier(StringRegistry* string_registry, std::string_view text) :
  string_registry(string_registry), text(text), scope_depth(-1), is_lhs(false) {
    push_scope(nullptr);
  }
  ~IdentifierClassifier() = default;

  Block* block(Block& block);
  Def* function_def(FunctionDef& def);
  Expr* identifier_reference_expr(IdentifierReferenceExpr& expr);

  IfStmt* if_stmt(IfStmt& stmt);
  ExprStmt* expr_stmt(ExprStmt& stmt);
  AssignmentStmt* assignment_stmt(AssignmentStmt& stmt);

  void if_branch(IfBranch& branch);

  const ParseErrors& get_errors() const {
    return errors;
  }

private:
  template <typename T>
  static void conditional_reset(std::unique_ptr<T>& source, T* maybe_new_ptr);

  void push_scope(const std::shared_ptr<ParseScope>& parse_scope);
  void pop_scope();

  IdentifierScope* scope_at(int index);
  const IdentifierScope* scope_at(int index) const;
  IdentifierScope* current_scope();
  const IdentifierScope* current_scope() const;

  Expr* identifier_reference_expr_lhs(IdentifierReferenceExpr& expr);
  Expr* identifier_reference_expr_rhs(IdentifierReferenceExpr& expr);

  void add_error(ParseError&& error);

  ParseError make_error_assignment_to_non_variable(const Token& at_token, int64_t identifier, IdentifierType present_type);

private:
  StringRegistry* string_registry;
  std::string_view text;

  std::vector<IdentifierScope> scopes;
  int scope_depth;
  bool is_lhs;

  ParseErrors errors;
};

template <typename T>
void IdentifierClassifier::conditional_reset(std::unique_ptr<T>& source, T* maybe_new_ptr) {
  if (source.get() != maybe_new_ptr) {
    source.reset(maybe_new_ptr);
  }
}

}