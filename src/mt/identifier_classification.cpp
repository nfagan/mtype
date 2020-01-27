#include "identifier_classification.hpp"
#include "ast_gen.hpp"
#include "string.hpp"
#include <cassert>

namespace mt {

void IdentifierScope::push_context() {
  contexts.emplace_back(IdentifierContext(current_context_depth()+1, context_uuid++));
}

int IdentifierScope::current_context_depth() const {
  return contexts.size();
}

const IdentifierScope::IdentifierContext* IdentifierScope::context_at_depth(int depth) const {
  if (depth < 1 || depth > int(contexts.size())) {
    return nullptr;
  }

  return &contexts[depth-1];
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
    auto* parent_info = lookup_variable(id, true);

    if (parent_info && parent_info->type == IdentifierType::variable_assignment_or_initialization) {
      //  Variable is shared from parent scope.
      IdentifierInfo new_info = *parent_info;
      new_info.context = current_context();
      classified_identifiers[id] = new_info;

      return AssignmentResult();

    } else {
      //  New variable assignment shadows parent declaration.
      auto variable_def = std::make_unique<VariableDef>(id);
      const auto type = IdentifierType::variable_assignment_or_initialization;

      IdentifierInfo new_info(type, current_context(), variable_def.get());
      classified_identifiers[id] = new_info;

      AssignmentResult result;
      result.was_initialization = true;
      result.variable_def = std::move(variable_def);

      return result;
    }
  }

  //  Otherwise, we must confirm that the previous usage of the identifier was a variable.
  if (!is_variable(info->type)) {
    //  This identifier was previously used as a different type of symbol.
    return AssignmentResult(false, info->type);
  }

  if (current_context_depth() <= info->context.depth) {
    //  The variable has now been assigned in a lower context depth, so is safe to reference from
    //  higher context depths from now on.
    info->context = current_context();
  }

  return AssignmentResult();
}

IdentifierScope::ReferenceResult IdentifierScope::register_identifier_reference(int64_t id) {
  //  @TODO: If an identifier used as a variable assignment has either a) been assigned at the
  //   current context or been assigned in all preceding child contexts, then it is ok to reference.

  const auto* info = lookup_variable(id, true);
  const auto* local_info = lookup_variable(id, false);

  if (!info) {
    //  This identifier has not been classified, and has not been assigned or initialized, so it is
    //  either an external function reference or a reference to a function in the current or
    //  a parent's scope.
    FunctionDef* maybe_function_def = lookup_function(id);
    auto type = maybe_function_def ? IdentifierType::resolved_local_function : IdentifierType::unresolved_external_function;
    IdentifierInfo new_info(type, current_context(), maybe_function_def);
    classified_identifiers[id] = new_info;
    return ReferenceResult(true, new_info);

  } else if (!local_info) {
    //  This is an identifier in a parent scope, and inherits its usage info.
    classified_identifiers[id] = *info;
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

    return ReferenceResult(success, *info);
  } else {
    return ReferenceResult(true, *info);
  }
}

FunctionDef* IdentifierScope::lookup_function(int64_t name, const std::shared_ptr<MatlabScope>& lookup_scope) {
  if (lookup_scope == nullptr) {
    return nullptr;
  }

  auto it = lookup_scope->local_functions.find(name);
  if (it == lookup_scope->local_functions.end()) {
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

bool IdentifierScope::has_variable(int64_t id, bool traverse_parent) {
  auto* info = lookup_variable(id, traverse_parent);
  if (!info) {
    return false;
  } else {
    return is_variable(info->type);
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

IdentifierClassifier::IdentifierClassifier(StringRegistry* string_registry, std::string_view text) :
string_registry(string_registry), text(text), scope_depth(-1) {
  //  Begin on rhs.
  push_rhs();
  //  New scope with no parent.
  push_scope(nullptr);
}

void IdentifierClassifier::push_context() {
  current_scope()->push_context();
}

void IdentifierClassifier::pop_context() {
  current_scope()->pop_context();
}

bool IdentifierClassifier::is_lhs() const {
  return expr_sides.back();
}

void IdentifierClassifier::push_lhs() {
  expr_sides.push_back(true);
}

void IdentifierClassifier::push_rhs() {
  expr_sides.push_back(false);
}

void IdentifierClassifier::pop_expr_side() {
  assert(!expr_sides.empty() && "No expr side to pop.");
  expr_sides.pop_back();
}

void IdentifierClassifier::push_scope(const std::shared_ptr<MatlabScope>& parse_scope) {
  int parent_index = scope_depth++;
  IdentifierScope new_scope(this, parse_scope, parent_index);

  if (scope_depth >= int(scopes.size())) {
    scopes.emplace_back(std::move(new_scope));
  } else {
    scopes[scope_depth] = std::move(new_scope);
  }

  error_identifiers_by_scope.emplace_back();
}

void IdentifierClassifier::pop_scope() {
  assert(scope_depth >= 0 && "Attempted to pop an empty scopes array.");
  scope_depth--;
  error_identifiers_by_scope.pop_back();
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

void IdentifierClassifier::register_variable_assignments(const Token& source_token, std::vector<int64_t>& identifiers) {
  for (const auto& id : identifiers) {
    auto assign_res = current_scope()->register_variable_assignment(id);
    if (!assign_res.success) {
      auto err = make_error_assignment_to_non_variable(source_token, id, assign_res.error_already_had_type);
      add_error_if_new_identifier(std::move(err), id);
    }
  }
}

namespace {
std::vector<int64_t> make_compound_identifier(const IdentifierReferenceExpr& expr, int64_t* end) {
  std::vector<int64_t> identifier_components;
  identifier_components.push_back(expr.primary_identifier);

  int64_t i = 0;
  const auto sz = int64_t(expr.subscripts.size());

  while (i < sz && expr.subscripts[i].method == SubscriptMethod::period) {
    const auto& sub = expr.subscripts[i];

    //  Period subscript expressions always have 1 argument.
    if (!sub.arguments[0]->append_to_compound_identifier(identifier_components)) {
      break;
    }

    i++;
  }

  *end = i;
  return identifier_components;
}
}

Expr* IdentifierClassifier::grouping_expr(GroupingExpr& expr) {
  for (auto& component : expr.components) {
    conditional_reset(component.expr, component.expr->accept(*this));
  }
  return &expr;
}

Expr* IdentifierClassifier::dynamic_field_reference_expr(DynamicFieldReferenceExpr& expr) {
  conditional_reset(expr.expr, expr.expr->accept(*this));
  return &expr;
}

Expr* IdentifierClassifier::identifier_reference_expr(IdentifierReferenceExpr& expr) {
  if (is_lhs()) {
    return identifier_reference_expr_lhs(expr);
  } else {
    return identifier_reference_expr_rhs(expr);
  }
}

Expr* IdentifierClassifier::identifier_reference_expr_lhs(mt::IdentifierReferenceExpr& expr) {
  //  In an assignment, the primary identifier reserves that identifier as a variable, and any
  //  compound identifiers deriving from it are thus also variables. E.g., if `a = struct()`, then
  //  `a.b.c` is also a variable.
  int64_t primary_identifier = expr.primary_identifier;

  auto primary_result = current_scope()->register_variable_assignment(primary_identifier);
  if (!primary_result.success) {
    auto err = make_error_assignment_to_non_variable(expr.source_token,
      primary_identifier, primary_result.error_already_had_type);
    add_error_if_new_identifier(std::move(err), primary_identifier);
  }

  if (primary_result.was_initialization && !expr.subscripts.empty()) {
    //  Implicit variable initialization of the form a.b = 3;
    auto err = make_error_implicit_variable_initialization(expr.source_token);
    add_error_if_new_identifier(std::move(err), primary_identifier);
  }

  if (primary_result.was_initialization) {
    //  Register the definition of this variable.
    assert(primary_result.variable_def.get() && "Expected allocated VariableDef object.");
    auto& parse_scope = current_scope()->parse_scope;
    parse_scope->register_local_variable(primary_identifier, std::move(primary_result.variable_def));
  }

  int64_t subscript_end;
  const auto compound_identifier_components = make_compound_identifier(expr, &subscript_end);

  //  Proceed beginning from the first non literal field reference subscript.
  push_rhs();
  subscripts(expr.subscripts, subscript_end);
  pop_expr_side();

  return &expr;
}

Expr* IdentifierClassifier::identifier_reference_expr_rhs(mt::IdentifierReferenceExpr& expr) {
  int64_t subscript_end;
  const auto compound_identifier_components = make_compound_identifier(expr, &subscript_end);

  const bool is_variable = current_scope()->has_variable(expr.primary_identifier, true);
  const int64_t primary_identifier = expr.primary_identifier;

  const auto primary_result = current_scope()->register_identifier_reference(primary_identifier);
  if (!primary_result.success) {
    auto err = make_error_variable_referenced_before_assignment(expr.source_token, primary_identifier);
    add_error_if_new_identifier(std::move(err), primary_identifier);
  }

  const bool is_compound_function_identifier = !is_variable && subscript_end > 1;
  int64_t function_name = primary_identifier;
  FunctionDef* function_def = nullptr;

  if (is_compound_function_identifier) {
    //  Presumed function reference with more than 1 identifier component, e.g. `a.b()`. Register
    //  the compound identifier as well.
    const int64_t compound_identifier = string_registry->make_registered_compound_identifier(compound_identifier_components);
    const auto compound_ref_result = current_scope()->register_identifier_reference(compound_identifier);
    if (!compound_ref_result.success) {
      auto err = make_error_variable_referenced_before_assignment(expr.source_token, compound_identifier);
      add_error_if_new_identifier(std::move(err), compound_identifier);
    }

    function_name = compound_identifier;
    function_def = compound_ref_result.info.function_def;

  } else if (!is_variable) {
    function_def = primary_result.info.function_def;
  }

  if (!is_variable) {
    auto ref_err = check_function_reference_subscript(expr, subscript_end);
    if (ref_err) {
      add_error(ref_err.rvalue());
    } else {
      //  Convert the identifier reference expression into a function call expression.
      return make_function_call_expr(expr, subscript_end, function_name, function_def);
    }
  }

  //  Proceed beginning from the first non literal field reference subscript.
  subscripts(expr.subscripts, subscript_end);

  return &expr;
}

FunctionCallExpr* IdentifierClassifier::make_function_call_expr(IdentifierReferenceExpr& from_expr,
                                                                int64_t subscript_end,
                                                                int64_t name,
                                                                FunctionDef* function_def) {
  const int64_t num_subs = from_expr.subscripts.size();
  std::vector<BoxedExpr> arguments;
  std::vector<Subscript> remaining_subscripts;

  if (subscript_end < num_subs) {
    auto& argument_subscript = from_expr.subscripts[subscript_end];
    assert(argument_subscript.method == SubscriptMethod::parens && "Expected parentheses subscript.");
    subscript(argument_subscript);

    arguments = std::move(argument_subscript.arguments);
  }

  for (int64_t i = subscript_end+1; i < num_subs; i++) {
    remaining_subscripts.emplace_back(std::move(from_expr.subscripts[i]));
  }

  subscripts(remaining_subscripts, 0);

  auto* node = new FunctionCallExpr(from_expr.source_token, function_def, name,
    std::move(arguments), std::move(remaining_subscripts));

  return node;
}

void IdentifierClassifier::subscripts(std::vector<Subscript>& subscripts, int64_t begin) {
  for (int64_t i = begin; i < int64_t(subscripts.size()); i++) {
    subscript(subscripts[i]);
  }
}

void IdentifierClassifier::subscript(Subscript& sub) {
  for (auto& arg : sub.arguments) {
    conditional_reset(arg, arg->accept(*this));
  }
}

void IdentifierClassifier::if_branch(IfBranch& branch) {
  conditional_reset(branch.condition_expr, branch.condition_expr->accept(*this));
  block_new_context(branch.block);
}

IfStmt* IdentifierClassifier::if_stmt(IfStmt& stmt) {
  if_branch(stmt.if_branch);
  for (auto& branch : stmt.elseif_branches) {
    if_branch(branch);
  }
  if (stmt.else_branch) {
    block_new_context(stmt.else_branch.value().block);
  }
  return &stmt;
}

AssignmentStmt* IdentifierClassifier::assignment_stmt(AssignmentStmt& stmt) {
  push_lhs();
  conditional_reset(stmt.to_expr, stmt.to_expr->accept(*this));
  pop_expr_side();
  conditional_reset(stmt.of_expr, stmt.of_expr->accept(*this));
  return &stmt;
}

ExprStmt* IdentifierClassifier::expr_stmt(ExprStmt& stmt) {
  conditional_reset(stmt.expr, stmt.expr->accept(*this));
  return &stmt;
}

Def* IdentifierClassifier::function_def(FunctionDef& def) {
  push_scope(def.scope);
  auto* scope = current_scope();

  for (const auto& it : scope->parse_scope->local_functions) {
    //  Local functions take precedence over variables.
    auto res = scope->register_identifier_reference(it.second->header.name);
    assert(res.success && "Failed to register local function.");
  }

  register_variable_assignments(def.header.name_token, def.header.inputs);
  register_variable_assignments(def.header.name_token, def.header.outputs);

  block_new_context(def.body);
  pop_scope();
  return &def;
}

Block* IdentifierClassifier::block(Block& block) {
  for (auto& node : block.nodes) {
    conditional_reset(node, node->accept(*this));
  }
  return &block;
}

void IdentifierClassifier::block_preserve_context(mt::BoxedBlock& blk) {
  conditional_reset(blk, block(*blk));
}

void IdentifierClassifier::block_new_context(mt::BoxedBlock& block) {
  push_context();
  block_preserve_context(block);
  pop_context();
}

void IdentifierClassifier::add_error(mt::ParseError&& error) {
  errors.emplace_back(error);
}

void IdentifierClassifier::add_error_if_new_identifier(mt::ParseError&& err, int64_t identifier) {
  if (!added_error_for_identifier(identifier)) {
    add_error(std::move(err));
    mark_error_identifier(identifier);
  }
}

bool IdentifierClassifier::added_error_for_identifier(int64_t identifier) const {
  return error_identifiers_by_scope.back().count(identifier) > 0;
}

void IdentifierClassifier::mark_error_identifier(int64_t identifier) {
  error_identifiers_by_scope.back().insert(identifier);
}

Optional<ParseError>
IdentifierClassifier::check_function_reference_subscript(const IdentifierReferenceExpr& expr, int64_t subscript_end) {
  const auto num_subs = int64_t(expr.subscripts.size());
  const auto& subs = expr.subscripts;

  if (num_subs > 0 && subscript_end < num_subs) {
    //  a{1}, a.b{1}, a.(b)
    if (subs[subscript_end].method != SubscriptMethod::parens) {
      return Optional<ParseError>(make_error_invalid_function_call_expr(expr.source_token));
    }
  }

  return NullOpt{};
}

ParseError IdentifierClassifier::make_error_assignment_to_non_variable(const Token& at_token,
                                                                       int64_t identifier,
                                                                       IdentifierType present_type) {
  auto identifier_str = std::string(string_registry->at(identifier));
  auto type_str = std::string(to_string(present_type));

  std::string msg = "The usage of `" + identifier_str + "` as a variable assignment target ";
  msg += "is inconsistent with its previous usage as a `" + type_str + "`.";

  return ParseError(text, at_token, msg);
}

ParseError IdentifierClassifier::make_error_variable_referenced_before_assignment(const Token& at_token, int64_t identifier) {
  auto identifier_str = std::string(string_registry->at(identifier));

  std::string msg = "The identifier `" + identifier_str + "`, apparently a variable, might be ";
  msg += "used before it is explicitly assigned a value.";

  return ParseError(text, at_token, msg);
}

ParseError IdentifierClassifier::make_error_implicit_variable_initialization(const mt::Token& at_token) {
  return ParseError(text, at_token, "Variables cannot be implicitly initialized with subscripts.");
}

ParseError IdentifierClassifier::make_error_invalid_function_call_expr(const Token& at_token) {
  return ParseError(text, at_token, "Functions cannot be `.` or `{}` referenced.");
}

ParseError IdentifierClassifier::make_error_invalid_function_index_expr(const Token& at_token) {
  return ParseError(text, at_token, "Function outputs cannot be `()` or `{}` referenced.");
}

}
