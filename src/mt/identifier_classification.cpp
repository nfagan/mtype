#include "identifier_classification.hpp"
#include "string.hpp"
#include <cassert>

namespace mt {

void VariableAssignmentContext::register_assignment(int64_t id, int64_t context_uuid) {
  if (context_uuids_by_identifier.count(id) == 0) {
    register_initialization(id, context_uuid);
  } else {
    context_uuids_by_identifier.at(id).insert(context_uuid);
  }
}

void VariableAssignmentContext::register_initialization(int64_t id, int64_t context_uuid) {
  assert(context_uuids_by_identifier.count(id) == 0 && "Identifier was already initialized.");
  std::set<int64_t> context_uuids;
  context_uuids.insert(context_uuid);
  context_uuids_by_identifier[id] = std::move(context_uuids);
}

void VariableAssignmentContext::register_child_context(int64_t context_uuid) {
  child_context_uuids.push_back(context_uuid);
}

void IdentifierScope::push_variable_assignment_context() {
  variable_assignment_contexts.emplace_back(int(variable_assignment_contexts.size())-1);
}

void IdentifierScope::pop_variable_assignment_context() {
  assert(!variable_assignment_contexts.empty() && "No variable assignment context to pop.");
  assert(!contexts.empty() && "No contexts exist.");

  auto curr_context = current_context();
  const auto& assign_context = variable_assignment_contexts.back();
  const auto& child_context_ids = assign_context.child_context_uuids;

  for (const auto& identifiers : assign_context.context_uuids_by_identifier) {
    const int64_t identifier = identifiers.first;
    const auto& present_in_contexts = identifiers.second;
    bool all_assigned = true;

    for (const auto& id : child_context_ids) {
      if (present_in_contexts.count(id) == 0) {
        all_assigned = false;
        break;
      }
    }

    if (all_assigned) {
      //  This identifier was assigned a value in all child contexts, so its visibility can be
      //  "promoted" to the parent context.
      assert(classified_identifiers.count(identifier) > 0 && "No identifier registered ??");
      auto var_parent_index = assign_context.parent_index;

      if (var_parent_index >= 0) {
        auto& var_parent_context = variable_assignment_contexts[var_parent_index];
        var_parent_context.register_assignment(identifier, curr_context.uuid);
      }

      auto& info = classified_identifiers.at(identifier);
      info.context = curr_context;
    }
  }

  variable_assignment_contexts.pop_back();
}

void IdentifierScope::push_context() {
  int64_t new_context_uuid = context_uuid++;

  if (!variable_assignment_contexts.empty()) {
    variable_assignment_contexts.back().register_child_context(new_context_uuid);
  }

  contexts.emplace_back(IdentifierContext(current_context_depth()+1, scope_depth, new_context_uuid));
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
  assert(!contexts.empty() && "No current context exists.");
  return contexts.back();
}

void IdentifierScope::pop_context() {
  assert(!contexts.empty() && "No context to pop.");
  contexts.pop_back();
}

IdentifierScope::AssignmentResult
IdentifierScope::register_variable_assignment(int64_t id, bool force_shadow_parent_assignment) {
  auto* info = lookup_variable(id, false);

  //  This identifier has not been classified. Register it as a variable assignment.
  if (!info) {
    auto* parent_info = lookup_variable(id, true);
    const bool is_parent_assignment = parent_info &&
      parent_info->type == IdentifierType::variable_assignment_or_initialization;

    if (is_parent_assignment && !force_shadow_parent_assignment) {
      //  Variable is shared from parent scope.
      IdentifierInfo new_info = *parent_info;
      new_info.context = current_context();
      classified_identifiers[id] = new_info;

      return AssignmentResult(new_info.variable_def);

    } else {
      //  New variable assignment shadows parent declaration. Caller needs to check for
      //  was_initialization to take ownership of variable_def.
      auto* variable_def = new VariableDef(id);
      const auto type = IdentifierType::variable_assignment_or_initialization;

      IdentifierInfo new_info(type, current_context(), variable_def);
      classified_identifiers[id] = new_info;

      AssignmentResult success_result(variable_def);
      success_result.was_initialization = true;

      if (!variable_assignment_contexts.empty()) {
        variable_assignment_contexts.back().register_assignment(id, current_context().uuid);
      }

      return success_result;
    }
  }

  //  Otherwise, we must confirm that the previous usage of the identifier was a variable.
  if (!is_variable(info->type)) {
    //  This identifier was previously used as a different type of symbol.
    return AssignmentResult(false, info->type, nullptr);
  }

  // Now we've successfully resolved the variable assignment.
  if (current_context_depth() <= info->context.depth) {
    //  The variable has now been assigned in a lower context depth or later context at the same
    //  depth, so it is safe to reference from higher context depths from now on.
    info->context = current_context();
  }

  if (!variable_assignment_contexts.empty()) {
    variable_assignment_contexts.back().register_assignment(id, current_context().uuid);
  }

  return AssignmentResult(info->variable_def);
}

namespace {
inline IdentifierType identifier_type_from_function_def(FunctionDef* def) {
  return def ? IdentifierType::local_function : IdentifierType::unresolved_external_function;
}
}

IdentifierScope::ReferenceResult IdentifierScope::register_compound_identifier_reference(int64_t id) {
  return register_identifier_reference(id, true);
}

IdentifierScope::ReferenceResult IdentifierScope::register_scalar_identifier_reference(int64_t id) {
  return register_identifier_reference(id, false);
}

IdentifierScope::ReferenceResult IdentifierScope::register_identifier_reference(int64_t id, bool is_compound) {
  const auto* info = lookup_variable(id, true);
  const auto* local_info = lookup_variable(id, false);

  if (!info) {
    //  This identifier has not been classified, and has not been assigned or initialized, so it is
    //  either an external function reference or a reference to a function in the current or
    //  a parent's scope.
    auto new_info = make_function_reference_identifier_info(id, lookup_local_function(id), is_compound);
    classified_identifiers[id] = new_info;
    return ReferenceResult(true, new_info);

  } else if (!local_info) {
    //  This is an identifier in a parent scope, but we need to check to see if any local functions
    //  take precedence over the parent's usage of the identifier.
    FunctionReference* maybe_function_ref = lookup_local_function(id);
    if (!maybe_function_ref) {
      //  No local function in the current scope, so it inherits the usage from the parent.
      classified_identifiers[id] = *info;
    } else {
      auto new_info = make_local_function_reference_identifier_info(maybe_function_ref);
      classified_identifiers[id] = new_info;
      return ReferenceResult(true, new_info);
    }
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
    } else {
      //  No classification exists at this context depth, but if it was registered in a parent
      //  scope, then it's OK to reference, regardless.
      //  @TODO: This is not quite correct. We must still make sure the variable was initialized
      //  in the context containing this scope, if this new scope is due to an anonymous function.
      success = info->context.scope_depth < current_context().scope_depth;
    }

    return ReferenceResult(success, *info);

  } else if (info->type == IdentifierType::unresolved_external_function && !info->is_compound_identifier) {
    //  Because functions can be overloaded and are dispatched depending on their argument types,
    //  we can't assume that `id` corresponds to an existing function -- we must make a new
    //  reference. The exception to this rule is that package functions and static methods referenced
    //  via a compound identifier cannot be dispatched, and so always refer to the same function.
    return ReferenceResult(true, make_external_function_reference_identifier_info(id, is_compound));

  } else {
    return ReferenceResult(true, *info);
  }
}

IdentifierScope::ReferenceResult
IdentifierScope::register_fully_qualified_import(int64_t complete_identifier, int64_t last_identifier_component) {
  auto* local_last_info = lookup_variable(last_identifier_component, false);
  if (local_last_info) {
    //  Imported identifier component should not already have been registered.
    return ReferenceResult(false, *local_last_info);
  }

  //  Check whether the complete identifier is already registered as an external function in a parent
  //  scope. If so, we can re-use the parent info.
  IdentifierInfo new_info;
  auto* parent_info = lookup_variable(complete_identifier, true);

  if (parent_info && parent_info->type == IdentifierType::unresolved_external_function) {
    new_info = *parent_info;
  } else {
    //  Register the reference as `last_identifier_component`, but create the function reference
    //  to `complete_identifier`.
    new_info = make_external_function_reference_identifier_info(complete_identifier, true);
  }

  classified_identifiers[last_identifier_component] = new_info;
  return ReferenceResult(true, new_info);
}

IdentifierScope::IdentifierInfo
IdentifierScope::make_external_function_reference_identifier_info(int64_t identifier, bool is_compound) {
  auto ref = classifier->function_registry->make_external_reference(identifier, matlab_scope);
  IdentifierInfo info(IdentifierType::unresolved_external_function, current_context(), ref);
  info.is_compound_identifier = is_compound;
  return info;
}

IdentifierScope::IdentifierInfo
IdentifierScope::make_local_function_reference_identifier_info(FunctionReference* ref) {
  auto type = identifier_type_from_function_def(ref->def);
  return IdentifierInfo(type, current_context(), ref);
}

IdentifierScope::IdentifierInfo
IdentifierScope::make_function_reference_identifier_info(int64_t identifier, FunctionReference* maybe_local_ref, bool is_compound) {
  if (maybe_local_ref) {
    return make_local_function_reference_identifier_info(maybe_local_ref);
  } else {
    return make_external_function_reference_identifier_info(identifier, is_compound);
  }
}

FunctionReference* IdentifierScope::lookup_local_function(int64_t name) const {
  return matlab_scope ? matlab_scope->lookup_local_function(name) : nullptr;
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
  return info ? is_variable(info->type) : false;
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

IdentifierClassifier::IdentifierClassifier(StringRegistry* string_registry,
                                           FunctionRegistry* function_registry,
                                           ClassDefStore* class_store,
                                           std::string_view text) :
string_registry(string_registry),
function_registry(function_registry),
class_store(class_store),
text(text),
scope_depth(-1) {
  //  Begin on rhs.
  push_rhs();
  //  New scope with no parent.
  push_scope(nullptr);
}

void IdentifierClassifier::register_new_context() {
  push_context();
  pop_context();
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
  IdentifierScope new_scope(this, parse_scope, parent_index, parent_index);

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

IdentifierScope::AssignmentResult
IdentifierClassifier::register_variable_assignment(const Token& source_token, int64_t primary_identifier) {
  auto primary_result = current_scope()->register_variable_assignment(primary_identifier);
  if (!primary_result.success) {
    const auto err_type = primary_result.error_already_had_type;
    auto err = make_error_assignment_to_non_variable(source_token, primary_identifier, err_type);
    add_error_if_new_identifier(std::move(err), primary_identifier);

  } else if (primary_result.was_initialization) {
    //  Register the definition of this variable, and take ownership of the definition.
    std::unique_ptr<VariableDef> variable_def(primary_result.variable_def);
    assert(variable_def.get() && "Expected newly allocated VariableDef object.");

    auto& parse_scope = current_scope()->matlab_scope;
    assert(parse_scope && "No parse scope.");
    parse_scope->register_local_variable(primary_identifier, std::move(variable_def));
  }

  return primary_result;
}

void IdentifierClassifier::register_imports(IdentifierScope* scope) {
  for (const auto& import : scope->matlab_scope->fully_qualified_imports) {
    //  Fully qualified imports take precedence over variables, but cannot shadow local functions.
    auto complete_identifier = string_registry->make_registered_compound_identifier(import.identifier_components);
    auto res = scope->register_fully_qualified_import(complete_identifier, import.identifier_components.back());

    if (!res.success) {
      add_error(make_error_shadowed_import(import.source_token, res.info.type));
    }
  }
}

void IdentifierClassifier::register_local_functions(IdentifierScope* scope) {
  for (const auto& it : scope->matlab_scope->local_functions) {
    //  Local functions take precedence over variables.
    auto res = scope->register_scalar_identifier_reference(it.second->def->header.name);
    if (!res.success || res.info.type != IdentifierType::local_function) {
      assert(res.success && "Failed to register local function.");
    }
  }
}

void IdentifierClassifier::register_function_parameters(const Token& source_token, std::vector<int64_t>& identifiers) {
  for (const auto& id : identifiers) {
    register_function_parameter(source_token, id);
  }
}

void IdentifierClassifier::register_function_parameter(const Token& source_token, int64_t id) {
  //  Input and output parameters always introduce new local variables.
  const bool force_shadow_parent_assignment = true;
  auto assign_res = current_scope()->register_variable_assignment(id, force_shadow_parent_assignment);

  if (!assign_res.success) {
    auto err = make_error_assignment_to_non_variable(source_token, id, assign_res.error_already_had_type);
    add_error_if_new_identifier(std::move(err), id);

  } else if (assign_res.was_initialization) {
    std::unique_ptr<VariableDef> variable_def(assign_res.variable_def);
    current_scope()->matlab_scope->register_local_variable(id, std::move(variable_def));
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

Expr* IdentifierClassifier::unary_operator_expr(UnaryOperatorExpr& expr) {
  conditional_reset(expr.expr, expr.expr->accept(*this));
  return &expr;
}

Expr* IdentifierClassifier::binary_operator_expr(BinaryOperatorExpr& expr) {
  conditional_reset(expr.left, expr.left->accept(*this));
  conditional_reset(expr.right, expr.right->accept(*this));
  return &expr;
}

Expr* IdentifierClassifier::dynamic_field_reference_expr(DynamicFieldReferenceExpr& expr) {
  conditional_reset(expr.expr, expr.expr->accept(*this));
  return &expr;
}

Expr* IdentifierClassifier::anonymous_function_expr(AnonymousFunctionExpr& expr) {
  push_scope(expr.scope);

  for (const auto& maybe_id : expr.input_identifiers) {
    if (maybe_id) {
      register_function_parameter(expr.source_token, maybe_id.value());
    }
  }

  conditional_reset(expr.expr, expr.expr->accept(*this));

  pop_scope();
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
  auto primary_result = register_variable_assignment(expr.source_token, expr.primary_identifier);
  if (primary_result.was_initialization && !expr.subscripts.empty()) {
    //  Implicit variable initialization of the form e.g. `a.b = 3;`
    auto err = make_error_implicit_variable_initialization(expr.source_token);
    add_error_if_new_identifier(std::move(err), expr.primary_identifier);
  }

  int64_t subscript_end;
  const auto compound_identifier_components = make_compound_identifier(expr, &subscript_end);

  //  Proceed beginning from the first non literal field reference subscript.
  push_rhs();
  subscripts(expr.subscripts, subscript_end);
  pop_expr_side();

  return new VariableReferenceExpr(expr.source_token, primary_result.variable_def,
    expr.primary_identifier, std::move(expr.subscripts), primary_result.was_initialization);
}

Expr* IdentifierClassifier::identifier_reference_expr_rhs(mt::IdentifierReferenceExpr& expr) {
  int64_t subscript_end;
  const auto compound_identifier_components = make_compound_identifier(expr, &subscript_end);

  const bool is_variable = current_scope()->has_variable(expr.primary_identifier, true);
  const int64_t primary_identifier = expr.primary_identifier;

  const auto primary_result = current_scope()->register_scalar_identifier_reference(primary_identifier);
  if (!primary_result.success) {
    auto err = make_error_variable_referenced_before_assignment(expr.source_token, primary_identifier);
    add_warning_if_new_identifier(std::move(err), primary_identifier);
  }

  const bool is_compound_function_identifier = !is_variable && subscript_end > 0;
  FunctionReference* function_reference = nullptr;

  if (is_compound_function_identifier) {
    //  Presumed function reference with more than 1 identifier component, e.g. `a.b()`. Register
    //  the compound identifier as well.
    const int64_t compound_identifier = string_registry->make_registered_compound_identifier(compound_identifier_components);
    const auto compound_ref_result = current_scope()->register_compound_identifier_reference(compound_identifier);
    if (!compound_ref_result.success) {
      auto err = make_error_variable_referenced_before_assignment(expr.source_token, compound_identifier);
      add_warning_if_new_identifier(std::move(err), compound_identifier);
    }

    function_reference = compound_ref_result.info.function_reference;
  } else if (!is_variable) {
    function_reference = primary_result.info.function_reference;
  }

  if (!is_variable) {
    auto ref_err = check_function_reference_subscript(expr, subscript_end);
    if (ref_err) {
      add_error(ref_err.rvalue());
    } else {
      //  Convert the identifier reference expression into a function call expression.
      return make_function_call_expr(expr, subscript_end, function_reference);
    }
  }

  //  Proceed beginning from the first non literal field reference subscript.
  subscripts(expr.subscripts, subscript_end);

  return new VariableReferenceExpr(expr.source_token, primary_result.info.variable_def,
    primary_identifier, std::move(expr.subscripts), false);
}

FunctionCallExpr* IdentifierClassifier::make_function_call_expr(IdentifierReferenceExpr& from_expr,
                                                                int64_t subscript_end,
                                                                FunctionReference* function_reference) {
  const int64_t num_subs = from_expr.subscripts.size();
  std::vector<BoxedExpr> arguments;
  std::vector<Subscript> remaining_subscripts;

  if (subscript_end < num_subs) {
    //  In an expression a.b.c(1, 2, 3), `argument_subscript` is (1, 2, 3)
    auto& argument_subscript = from_expr.subscripts[subscript_end];
    assert(argument_subscript.method == SubscriptMethod::parens && "Expected parentheses subscript.");
    subscript(argument_subscript);

    //  The "subscript arguments" are actually just the function arguments.
    arguments = std::move(argument_subscript.arguments);
  }

  //  In a continued reference expression of the form e.g. `a.b.c(1, 2, 3).('a').b`, the subscripts
  //  `.('a')` and `.b` must be traversed.
  for (int64_t i = subscript_end+1; i < num_subs; i++) {
    remaining_subscripts.emplace_back(std::move(from_expr.subscripts[i]));
  }

  subscripts(remaining_subscripts, 0);

  return new FunctionCallExpr(from_expr.source_token, function_reference,
    std::move(arguments), std::move(remaining_subscripts));
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
  current_scope()->push_variable_assignment_context();

  if_branch(stmt.if_branch);
  for (auto& branch : stmt.elseif_branches) {
    if_branch(branch);
  }
  if (stmt.else_branch) {
    block_new_context(stmt.else_branch.value().block);
  } else {
    register_new_context();
  }

  current_scope()->pop_variable_assignment_context();
  return &stmt;
}

AssignmentStmt* IdentifierClassifier::assignment_stmt(AssignmentStmt& stmt) {
  conditional_reset(stmt.of_expr, stmt.of_expr->accept(*this));
  push_lhs();
  conditional_reset(stmt.to_expr, stmt.to_expr->accept(*this));
  pop_expr_side();

  return &stmt;
}

ExprStmt* IdentifierClassifier::expr_stmt(ExprStmt& stmt) {
  conditional_reset(stmt.expr, stmt.expr->accept(*this));
  return &stmt;
}

ForStmt* IdentifierClassifier::for_stmt(ForStmt& stmt) {
  register_variable_assignment(stmt.source_token, stmt.loop_variable_identifier);
  conditional_reset(stmt.loop_variable_expr, stmt.loop_variable_expr->accept(*this));
  block_new_context(stmt.body);
  return &stmt;
}

WhileStmt* IdentifierClassifier::while_stmt(WhileStmt& stmt) {
  conditional_reset(stmt.condition_expr, stmt.condition_expr->accept(*this));
  block_new_context(stmt.body);
  return &stmt;
}

SwitchStmt* IdentifierClassifier::switch_stmt(SwitchStmt& stmt) {
  current_scope()->push_variable_assignment_context();

  conditional_reset(stmt.condition_expr, stmt.condition_expr->accept(*this));
  for (auto& case_block : stmt.cases) {
    conditional_reset(case_block.expr, case_block.expr->accept(*this));
    block_new_context(case_block.block);
  }
  if (stmt.otherwise) {
    block_new_context(stmt.otherwise);
  } else {
    register_new_context();
  }

  current_scope()->pop_variable_assignment_context();
  return &stmt;
}

VariableDeclarationStmt* IdentifierClassifier::variable_declaration_stmt(VariableDeclarationStmt& stmt) {
  for (const int64_t identifier : stmt.identifiers) {
    const auto assign_res = register_variable_assignment(stmt.source_token, identifier);
    if (!assign_res.was_initialization) {
      add_error(make_error_pre_declared_qualified_variable(stmt.source_token, identifier));
    }
  }

  return &stmt;
}

FunctionReference* IdentifierClassifier::function_reference(FunctionReference& ref) {
  push_scope(ref.scope);
  auto* def = ref.def;
  auto* scope = current_scope();

  register_local_functions(scope);
  register_imports(scope);
  register_function_parameters(def->header.name_token, def->header.inputs);
  register_function_parameters(def->header.name_token, def->header.outputs);

  block_new_context(def->body);
  pop_scope();
  return &ref;
}

ClassDefReference* IdentifierClassifier::class_def_reference(ClassDefReference& ref) {
  auto& def = class_store->lookup_class(ref.handle);

  for (auto& method_block : def.method_blocks) {
    for (auto& function_ref : method_block.definitions) {
      conditional_reset(function_ref, function_ref->accept(*this));
    }
  }

  return &ref;
}

void IdentifierClassifier::transform_root(BoxedRootBlock& block) {
  conditional_reset(block, block->accept(*this));
}

RootBlock* IdentifierClassifier::root_block(RootBlock& block) {
  push_scope(block.scope);
  conditional_reset(block.block, block.block->accept(*this));
  pop_scope();
  return &block;
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

void IdentifierClassifier::add_warning_if_new_identifier(mt::ParseError&& err, int64_t identifier) {
  if (!added_error_for_identifier(identifier)) {
    warnings.emplace_back(std::move(err));
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
  msg += "referenced before it is explicitly assigned a value.";

  return ParseError(text, at_token, msg);
}

ParseError IdentifierClassifier::make_error_implicit_variable_initialization(const mt::Token& at_token) {
  return ParseError(text, at_token, "Variables cannot be implicitly initialized with subscripts.");
}

ParseError IdentifierClassifier::make_error_invalid_function_call_expr(const Token& at_token) {
  return ParseError(text, at_token, "Functions cannot be `.` or `{}` referenced.");
}

ParseError IdentifierClassifier::make_error_shadowed_import(const Token& at_token, IdentifierType present_type) {
  auto type_str = std::string(to_string(present_type));
  std::string msg = "Import is shadowed by identifier of type `" + type_str + "`.";
  return ParseError(text, at_token, msg);
}

ParseError IdentifierClassifier::make_error_pre_declared_qualified_variable(const Token& at_token, int64_t identifier) {
  auto identifier_str = std::string(string_registry->at(identifier));
  std::string msg = "The declaration of identifier `" + identifier_str + "` must precede its initialization.";
  return ParseError(text, at_token, msg);
}

}
