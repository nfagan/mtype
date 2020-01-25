#include "identifier_classification.hpp"
#include "ast_gen.hpp"
#include "string.hpp"
#include <cassert>

namespace mt {

void IdentifierScope::push_context() {
  contexts.emplace_back(IdentifierContext(current_context_depth(), context_uuid++));
}

int IdentifierScope::current_context_depth() const {
  return contexts.size();
}

const IdentifierScope::IdentifierContext* IdentifierScope::context_at_depth(int depth) const {
  if (depth >= int(contexts.size())) {
    return nullptr;
  }

  return &contexts[depth];
}

IdentifierScope::IdentifierContext IdentifierScope::current_context() const {
  assert(contexts.size() > 0 && "No current context exists.");
  return contexts.back();
}

void IdentifierScope::pop_context() {
  assert(contexts.size() > 0 && "No context to pop.");
  contexts.pop_back();
}

IdentifierScope::AssignmentResult IdentifierScope::register_variable_assignment(int64_t id) {
  auto* info = lookup_variable(id, false);

  //  This identifier has not been classified. Register it as a variable assignment.
  if (!info) {
    IdentifierInfo new_info(IdentifierType::variable_assignment_or_initialization, current_context());
    classified_identifiers[id] = new_info;
    return AssignmentResult();
  }

  //  Otherwise, we must confirm that the previous usage of the identifier was a variable.
  if (!is_variable(info->type)) {
    //  This identifier was previously used as a different type of symbol.
    return AssignmentResult(false, info->type);
  }

  if (current_context_depth() < info->context.depth) {
    //  The variable has now been assigned in a lower context depth, so is safe to reference from
    //  higher context depths from now on.
    info->context = current_context();
  }

  return AssignmentResult();
}

IdentifierScope::ReferenceResult IdentifierScope::register_identifier_reference(int64_t id) {
  const auto* info = lookup_variable(id, true);

  if (!info) {
    //  This identifier has not been classified, and has not been assigned or initialized, so it is
    //  either an external function reference or a reference to a function in the current or
    //  a parent's scope.
    FunctionDef* maybe_function_def = lookup_function(id);
    auto type = maybe_function_def ? IdentifierType::resolved_local_function : IdentifierType::unresolved_external_function;
    IdentifierInfo new_info(type, current_context(), maybe_function_def);
    classified_identifiers[id] = new_info;
    return ReferenceResult(true, type);
  }

  //  Otherwise, this identifier exists either in the current or a parent's scope.
  if (info->type == IdentifierType::variable_assignment_or_initialization) {
    //  The identifier was previously classified as a variable, so a valid reference requires that
    //  it was assigned / initialized in a context at least as low as the current one, and whose
    //  parent context is the same as the parent of the current context.
    const auto* current_context_at_info_depth = context_at_depth(info->context.depth);
    bool success = false;

    if (current_context_at_info_depth) {
      success = info->context.uuid == current_context_at_info_depth->uuid;
    }

    return ReferenceResult(success, info->type);
  } else {
    return ReferenceResult(true, info->type);
  }
}

FunctionDef* IdentifierScope::lookup_function(int64_t name, const std::shared_ptr<ParseScope>& lookup_scope) {
  if (lookup_scope == nullptr) {
    return nullptr;
  }

  auto it = lookup_scope->functions.find(name);
  if (it == lookup_scope->functions.end()) {
    return lookup_function(name, lookup_scope->parent);
  } else {
    return it->second;
  }
}

FunctionDef* IdentifierScope::lookup_function(int64_t name) const {
  return lookup_function(name, parse_scope);
}

IdentifierScope::IdentifierInfo* IdentifierScope::lookup_variable(int64_t id, bool traverse_parent) {
  auto it = classified_identifiers.find(id);
  if (it == classified_identifiers.end()) {
    return traverse_parent && has_parent() ? parent()->lookup_variable(id, true) : nullptr;
  } else {
    return &it->second;
  }
}

bool IdentifierScope::has_parent() const {
  return parent_index >= 0;
}

const IdentifierScope* IdentifierScope::parent() const {
  return parent_index < 0 ? nullptr : classifier->scope_at(parent_index);
}

IdentifierScope* IdentifierScope::parent() {
  return parent_index < 0 ? nullptr : classifier->scope_at(parent_index);
}

void IdentifierClassifier::push_scope(const std::shared_ptr<ParseScope>& parse_scope) {
  int parent_index = scope_depth++;
  IdentifierScope new_scope(this, parse_scope, parent_index);

  if (scope_depth >= int(scopes.size())) {
    scopes.emplace_back(std::move(new_scope));
  } else {
    scopes[scope_depth] = std::move(new_scope);
  }
}

void IdentifierClassifier::pop_scope() {
  assert(scope_depth >= 0 && "Attempted to pop an empty scopes array.");
  scope_depth--;
}

IdentifierScope* IdentifierClassifier::scope_at(int index) {
  return index < 0 ? nullptr : &scopes[index];
}

const IdentifierScope* IdentifierClassifier::scope_at(int index) const {
  return index < 0 ? nullptr : &scopes[index];
}

const IdentifierScope* IdentifierClassifier::current_scope() const {
  return scope_at(scope_depth);
}

IdentifierScope* IdentifierClassifier::current_scope() {
  return scope_at(scope_depth);
}

Expr* IdentifierClassifier::identifier_reference_expr(IdentifierReferenceExpr& expr) {
  if (is_lhs) {
    return identifier_reference_expr_lhs(expr);
  } else {
    return identifier_reference_expr_rhs(expr);
  }
}

namespace {
std::vector<int64_t> make_prefix(const IdentifierReferenceExpr& expr) {
  std::vector<int64_t> prefix_components;
  prefix_components.push_back(expr.primary_identifier);

  for (const auto& sub : expr.subscripts) {
    if (sub.method != SubscriptMethod::period || !sub.arguments[0]->build_prefix(prefix_components)) {
      break;
    }
  }

  return prefix_components;
}
}

Expr* IdentifierClassifier::identifier_reference_expr_lhs(mt::IdentifierReferenceExpr& expr) {
  //  In an assignment, the longest identifier prefix component, as well as the primary
  //  identifier, are variables. E.g., in a.b.c(1, 2) = 3; `a.b.c` is a "variable", in the sense
  //  that it would shadow a package or static method function accessible by the same name, and
  //  `a` is definitely a variable since it is an assigment target. All sub-components, e.g.,
  //  `a.b` are also variables.
  auto primary_result = current_scope()->register_variable_assignment(expr.primary_identifier);
  if (!primary_result.success) {
    add_error(make_error_assignment_to_non_variable(expr.source_token,
      expr.primary_identifier, primary_result.error_already_had_type));
  }

  //  The longest prefix must be registered as well, if one exists.
  if (!expr.subscripts.empty()) {
    const auto prefix_components = make_prefix(expr);

    for (int64_t i = prefix_components.size(); i > 1; i--) {
      auto prefixed_identifier = string_registry->make_registered_compound_identifier(prefix_components, i);
      auto prefix_result = current_scope()->register_variable_assignment(prefixed_identifier);
      if (!prefix_result.success) {
        add_error(make_error_assignment_to_non_variable(expr.source_token,
          prefixed_identifier, prefix_result.error_already_had_type));
      }
    }
  }

  return &expr;
}

Expr* IdentifierClassifier::identifier_reference_expr_rhs(mt::IdentifierReferenceExpr& expr) {
  int64_t identifier = expr.primary_identifier;

  if (!expr.subscripts.empty()) {
    const auto prefix_components = make_prefix(expr);
    identifier = string_registry->make_registered_compound_identifier(prefix_components, prefix_components.size());
  }

#if 1
  auto ref_result = current_scope()->register_identifier_reference(identifier);
  auto ident_str = string_registry->at(expr.primary_identifier);

  if (!ref_result.success) {
    std::cout << "Variable used before it was defined: " << ident_str << std::endl;
  } else {
    std::cout << "Successfully registered reference for: " << ident_str;
    std::cout << ", with type: " << to_string(ref_result.classified_type) << std::endl;
  }
#endif

  return &expr;
}

void IdentifierClassifier::if_branch(IfBranch& branch) {
  conditional_reset(branch.condition_expr, branch.condition_expr->accept(*this));
  conditional_reset(branch.block, branch.block->accept(*this));
}

IfStmt* IdentifierClassifier::if_stmt(IfStmt& stmt) {
  if_branch(stmt.if_branch);
  for (auto& branch : stmt.elseif_branches) {
    if_branch(branch);
  }
  if (stmt.else_branch) {
    auto branch = std::move(stmt.else_branch.rvalue());
    conditional_reset(branch.block, branch.block->accept(*this));
    stmt.else_branch = std::move(branch);
  }
  return &stmt;
}

AssignmentStmt* IdentifierClassifier::assignment_stmt(AssignmentStmt& stmt) {
  is_lhs = true;
  conditional_reset(stmt.to_expr, stmt.to_expr->accept(*this));
  is_lhs = false;
  conditional_reset(stmt.of_expr, stmt.of_expr->accept(*this));
  return &stmt;
}

ExprStmt* IdentifierClassifier::expr_stmt(ExprStmt& stmt) {
  conditional_reset(stmt.expr, stmt.expr->accept(*this));
  return &stmt;
}

Def* IdentifierClassifier::function_def(FunctionDef& def) {
  push_scope(def.scope);

  for (const auto& id : def.header.inputs) {
    current_scope()->register_variable_assignment(id);
  }
  for (const auto& id : def.header.outputs) {
    current_scope()->register_variable_assignment(id);
  }

  conditional_reset(def.body, def.body->accept(*this));
  pop_scope();
  return &def;
}

Block* IdentifierClassifier::block(Block& block) {
  current_scope()->push_context();

  for (auto& node : block.nodes) {
    conditional_reset(node, node->accept(*this));
  }

  current_scope()->pop_context();
  return &block;
}

void IdentifierClassifier::add_error(mt::ParseError&& error) {
  errors.emplace_back(error);
}

ParseError IdentifierClassifier::make_error_assignment_to_non_variable(const Token& at_token,
                                                                       int64_t identifier,
                                                                       IdentifierType present_type) {
  auto identifier_str = std::string(string_registry->at(identifier));
  auto type_str = std::string(to_string(present_type));

  std::string msg = "The usage of `" + identifier_str + "` as a variable assignment target";
  msg += " is inconsistent with its\nprevious usage as a `" + type_str + "`.";

  return ParseError(text, at_token, msg);
}

}
