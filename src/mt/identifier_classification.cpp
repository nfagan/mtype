#include "identifier_classification.hpp"
#include "string.hpp"
#include <cassert>

namespace mt {

void VariableAssignmentContext::register_assignment(const MatlabIdentifier& id, int64_t context_uuid) {
  if (context_uuids_by_identifier.count(id) == 0) {
    register_initialization(id, context_uuid);
  } else {
    context_uuids_by_identifier.at(id).insert(context_uuid);
  }
}

void VariableAssignmentContext::register_initialization(const MatlabIdentifier& id, int64_t context_uuid) {
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
    const auto& identifier = identifiers.first;
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
IdentifierScope::register_variable_assignment(Store::Write& variable_writer,
                                              const MatlabIdentifier& id,
                                              bool force_shadow_parent_assignment) {
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

      return AssignmentResult(new_info.variable_def_handle);

    } else {
      //  New variable assignment shadows parent declaration. Caller needs to check for
      //  was_initialization.
      VariableDef variable_def(id);

      auto def_handle = variable_writer.emplace_definition(std::move(variable_def));
      const auto type = IdentifierType::variable_assignment_or_initialization;

      IdentifierInfo new_info(id, type, current_context(), def_handle);
      classified_identifiers[id] = new_info;

      AssignmentResult success_result(def_handle);
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
    return AssignmentResult(false, info->type, VariableDefHandle());
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

  return AssignmentResult(info->variable_def_handle);
}

namespace {
inline IdentifierType identifier_type_from_function_def(FunctionDefHandle def) {
  return def.is_valid() ? IdentifierType::local_function : IdentifierType::unresolved_external_function;
}
}

IdentifierScope::ReferenceResult
IdentifierScope::register_identifier_reference(Store::Write& writer, const MatlabIdentifier& id) {
  const auto* info = lookup_variable(id, true);
  const auto* local_info = lookup_variable(id, false);

  if (!info) {
    //  This identifier has not been classified, and has not been assigned or initialized, so it is
    //  either an external function reference or a reference to a function in the current or
    //  a parent's scope.
    const auto local_func_handle = lookup_local_function(writer, id);
    auto new_info = make_function_reference_identifier_info(writer, id, local_func_handle);
    classified_identifiers[id] = new_info;
    return ReferenceResult(true, new_info);

  } else if (!local_info) {
    //  This is an identifier in a parent scope, but we need to check to see if any local functions
    //  take precedence over the parent's usage of the identifier.
    FunctionReferenceHandle maybe_function_ref = lookup_local_function(writer, id);
    if (!maybe_function_ref.is_valid()) {
      //  No local function in the current scope, so it inherits the usage from the parent.
      classified_identifiers[id] = *info;
    } else {
      auto new_info = make_local_function_reference_identifier_info(writer, id, maybe_function_ref);
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

  } else if (info->type == IdentifierType::unresolved_external_function && !info->source.is_compound()) {
    //  Because functions can be overloaded and are dispatched depending on their argument types,
    //  we can't assume that `id` corresponds to an existing function -- we must make a new
    //  reference. The exception to this rule is that package functions and static methods referenced
    //  via a compound identifier cannot be dispatched, and so always refer to the same function.
    return ReferenceResult(true, make_external_function_reference_identifier_info(writer, id));

  } else {
    return ReferenceResult(true, *info);
  }
}

IdentifierScope::ReferenceResult
IdentifierScope::register_fully_qualified_import(Store::Write& writer,
                                                 const MatlabIdentifier& complete_identifier,
                                                 const MatlabIdentifier& last_identifier_component) {
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
    new_info = make_external_function_reference_identifier_info(writer, complete_identifier);
  }

  classified_identifiers[last_identifier_component] = new_info;
  return ReferenceResult(true, new_info);
}

IdentifierScope::IdentifierInfo
IdentifierScope::make_external_function_reference_identifier_info(Store::Write& writer,
                                                                  const MatlabIdentifier& id) {
  auto ref = writer.make_external_reference(id, matlab_scope_handle);
  IdentifierInfo info(id, IdentifierType::unresolved_external_function, current_context(), ref);
  return info;
}

IdentifierScope::IdentifierInfo
IdentifierScope::make_local_function_reference_identifier_info(Store::Write& writer,
                                                               const MatlabIdentifier& id,
                                                               FunctionReferenceHandle ref_handle) {
  auto& ref = writer.at(ref_handle);
  auto type = identifier_type_from_function_def(ref.def_handle);
  return IdentifierInfo(id, type, current_context(), ref_handle);
}

IdentifierScope::IdentifierInfo
IdentifierScope::make_function_reference_identifier_info(Store::Write& writer,
                                                         const MatlabIdentifier& identifier,
                                                         FunctionReferenceHandle maybe_local_ref) {
  if (maybe_local_ref.is_valid()) {
    return make_local_function_reference_identifier_info(writer, identifier, maybe_local_ref);
  } else {
    return make_external_function_reference_identifier_info(writer, identifier);
  }
}

FunctionReferenceHandle IdentifierScope::lookup_local_function(const Store::Write& writer, const MatlabIdentifier& name) const {
  if (matlab_scope_handle.is_valid()) {
    return writer.lookup_local_function(matlab_scope_handle, name);
  } else {
    return FunctionReferenceHandle();
  }
}

IdentifierScope::IdentifierInfo* IdentifierScope::lookup_variable(const MatlabIdentifier& id, bool traverse_parent) {
  auto it = classified_identifiers.find(id);
  if (it == classified_identifiers.end()) {
    return traverse_parent && has_parent() ? parent()->lookup_variable(id, true) : nullptr;
  } else {
    return &it->second;
  }
}

bool IdentifierScope::has_variable(const MatlabIdentifier& id, bool traverse_parent) {
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
                                           Store* store,
                                           const CodeFileDescriptor* file_descriptor,
                                           std::string_view text) :
  string_registry(string_registry),
  store(store),
  file_descriptor(file_descriptor),
  text(text),
  scope_depth(-1) {
  //  Begin on rhs.
  expr_sides.push_rhs();
  //  Begin in non-superclass method application, etc.
  class_state.push_default_state();
  //  New scope with no parent.
  push_scope(MatlabScopeHandle());
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

void IdentifierClassifier::push_scope(const MatlabScopeHandle& parse_scope_handle) {
  int parent_index = scope_depth++;
  IdentifierScope new_scope(this, parse_scope_handle, parent_index, parent_index);

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
IdentifierClassifier::register_variable_assignment(const Token& source_token, const MatlabIdentifier& primary_identifier) {
  Store::Write read_write(*store);

  auto primary_result = current_scope()->register_variable_assignment(read_write, primary_identifier);
  if (!primary_result.success) {
    const auto err_type = primary_result.error_already_had_type;
    auto err = make_error_assignment_to_non_variable(source_token, primary_identifier, err_type);
    add_error_if_new_identifier(std::move(err), primary_identifier);

  } else if (primary_result.was_initialization) {
    auto& parse_scope = read_write.at(current_scope()->matlab_scope_handle);
    parse_scope.register_local_variable(primary_identifier, primary_result.variable_def_handle);
  }

  return primary_result;
}

void IdentifierClassifier::register_imports(IdentifierScope* scope) {
  Store::Write read_write(*store);

  const auto& matlab_scope = read_write.at(scope->matlab_scope_handle);

  for (const auto& import : matlab_scope.fully_qualified_imports) {
    //  Fully qualified imports take precedence over variables, but cannot shadow local functions.
    auto complete_identifier_id = string_registry->make_registered_compound_identifier(import.identifier_components);

    MatlabIdentifier complete_identifier(complete_identifier_id, import.identifier_components.size());
    MatlabIdentifier import_alias(import.identifier_components.back());

    auto res = scope->register_fully_qualified_import(read_write, complete_identifier, import_alias);

    if (!res.success) {
      add_error(make_error_shadowed_import(import.source_token, res.info.type));
    }
  }
}

void IdentifierClassifier::register_local_functions(IdentifierScope* scope) {
  Store::Write read_write(*store);
  const auto& matlab_scope = read_write.at(scope->matlab_scope_handle);

  for (const auto& it : matlab_scope.local_functions) {
    //  Local functions take precedence over variables.
    const auto& ref_handle = it.second;
    const auto& def_handle = read_write.at(ref_handle).def_handle;
    const auto& def = read_write.at(def_handle);

    auto res = scope->register_identifier_reference(read_write, def.header.name);
    if (!res.success || res.info.type != IdentifierType::local_function) {
      //  It's a bug if we fail to register a local function.
      assert(res.success && "Failed to register local function.");
    }
  }
}

void IdentifierClassifier::register_function_parameters(Store::Write& read_write,
                                                        const Token& source_token,
                                                        const std::vector<MatlabIdentifier>& identifiers) {
  for (const auto& id : identifiers) {
    register_function_parameter(read_write, source_token, id);
  }
}

void IdentifierClassifier::register_function_parameters(Store::Write& read_write,
                                                        const Token& source_token,
                                                        const std::vector<FunctionInputParameter>& inputs) {
  for (const auto& input : inputs) {
    if (!input.is_ignored) {
      register_function_parameter(read_write, source_token, input.name);
    }
  }
}

void IdentifierClassifier::register_function_parameter(Store::Write& read_write,
                                                       const Token& source_token,
                                                       const MatlabIdentifier& id) {
  //  Input and output parameters always introduce new local variables.
  const bool force_shadow_parent_assignment = true;
  auto assign_res =
    current_scope()->register_variable_assignment(read_write, id, force_shadow_parent_assignment);

  if (!assign_res.success) {
    auto err = make_error_assignment_to_non_variable(source_token, id, assign_res.error_already_had_type);
    add_error_if_new_identifier(std::move(err), id);

  } else if (assign_res.was_initialization) {
    auto& matlab_scope = read_write.at(current_scope()->matlab_scope_handle);
    matlab_scope.register_local_variable(id, assign_res.variable_def_handle);
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

Expr* IdentifierClassifier::function_reference_expr(FunctionReferenceExpr& expr) {
  IdentifierScope::ReferenceResult ref_result;
  const auto& name = expr.identifier;

  {
    Store::Write read_write(*store);
    ref_result = current_scope()->register_identifier_reference(read_write, name);
  }

  if (!is_function(ref_result.info.type)) {
    const auto& source_token = expr.source_token;
    const auto type = ref_result.info.type;

    auto err = make_error_function_reference_to_non_function(source_token, name, type);
    add_error_if_new_identifier(std::move(err), name);

  } else {
    expr.handle = ref_result.info.function_reference;
  }

  return &expr;
}

Expr* IdentifierClassifier::anonymous_function_expr(AnonymousFunctionExpr& expr) {
  push_scope(expr.scope_handle);

  {
    Store::Write writer(*store);
    register_function_parameters(writer, expr.source_token, expr.inputs);
  }

  conditional_reset(expr.expr, expr.expr->accept(*this));
  pop_scope();
  return &expr;
}

Expr* IdentifierClassifier::presumed_superclass_method_reference_expr(PresumedSuperclassMethodReferenceExpr& expr) {
  class_state.push_superclass_method_application();
  conditional_reset(expr.superclass_reference_expr, expr.superclass_reference_expr->accept(*this));
  class_state.pop_superclass_method_application();
  return &expr;
}

Expr* IdentifierClassifier::identifier_reference_expr(IdentifierReferenceExpr& expr) {
  if (expr_sides.is_lhs()) {
    return identifier_reference_expr_lhs(expr);

  } else if (class_state.is_within_superclass_method_application()) {
    return superclass_method_application(expr);

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
  const auto compound_identifier_components = expr.make_compound_identifier(&subscript_end);

  //  Proceed beginning from the first non literal field reference subscript.
  expr_sides.push_rhs();
  subscripts(expr.subscripts, subscript_end);
  expr_sides.pop_side();

  return new VariableReferenceExpr(expr.source_token, primary_result.variable_def_handle,
    expr.primary_identifier, std::move(expr.subscripts), primary_result.was_initialization);
}

Expr* IdentifierClassifier::identifier_reference_expr_rhs(mt::IdentifierReferenceExpr& expr) {
  int64_t subscript_end;
  const auto compound_identifier_components = expr.make_compound_identifier(&subscript_end);

  const bool is_variable = current_scope()->has_variable(expr.primary_identifier, true);
  const auto& primary_identifier = expr.primary_identifier;

  auto* scope = current_scope();

  IdentifierScope::ReferenceResult primary_result;

  {
    Store::Write read_write(*store);

    primary_result =
      scope->register_identifier_reference(read_write, primary_identifier);

    if (!primary_result.success) {
      auto err = make_error_variable_referenced_before_assignment(expr.source_token, primary_identifier);
      add_warning_if_new_identifier(std::move(err), primary_identifier);
    }
  }

  const bool is_compound_function_identifier = !is_variable && subscript_end > 0;
  FunctionReferenceHandle function_reference;

  if (is_compound_function_identifier) {
    //  Presumed function reference with more than 1 identifier component, e.g. `a.b()`. Register
    //  the compound identifier as well.
    Store::Write read_write(*store);

    const int64_t compound_identifier_id =
      string_registry->make_registered_compound_identifier(compound_identifier_components);
    const MatlabIdentifier compound_identifier(compound_identifier_id, compound_identifier_components.size());

    const auto compound_ref_result =
      scope->register_identifier_reference(read_write, compound_identifier);

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

  return new VariableReferenceExpr(expr.source_token, primary_result.info.variable_def_handle,
    primary_identifier, std::move(expr.subscripts), false);
}

Expr* IdentifierClassifier::superclass_method_application(mt::IdentifierReferenceExpr& expr) {
  int64_t subscript_end;
  const auto compound_identifier_components = expr.make_compound_identifier(&subscript_end);

  auto ref_err = check_function_reference_subscript(expr, subscript_end);
  if (ref_err) {
    add_error(ref_err.rvalue());
    return &expr;
  }

  const auto scope_handle = current_scope()->matlab_scope_handle;

  const auto function_name_id = string_registry->make_registered_compound_identifier(compound_identifier_components);
  MatlabIdentifier function_name(function_name_id, compound_identifier_components.size());
  FunctionReferenceHandle function_reference_handle;

  {
    Store::Write read_write(*store);
    function_reference_handle = read_write.make_external_reference(function_name, scope_handle);
  }

  class_state.push_non_superclass_method_application();
  auto* func_node = make_function_call_expr(expr, subscript_end, function_reference_handle);
  class_state.pop_superclass_method_application();

  return func_node;
}

FunctionCallExpr* IdentifierClassifier::make_function_call_expr(IdentifierReferenceExpr& from_expr,
                                                                int64_t subscript_end,
                                                                FunctionReferenceHandle function_reference) {
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
  expr_sides.push_lhs();
  conditional_reset(stmt.to_expr, stmt.to_expr->accept(*this));
  expr_sides.pop_side();

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
  for (const auto& identifier : stmt.identifiers) {
    const auto assign_res = register_variable_assignment(stmt.source_token, identifier);
    if (!assign_res.was_initialization) {
      add_error(make_error_pre_declared_qualified_variable(stmt.source_token, identifier));
    }
  }

  return &stmt;
}

FunctionDefNode* IdentifierClassifier::function_def_node(FunctionDefNode& def_node) {
  push_scope(def_node.scope_handle);

  //  Local functions take precedence over imports
  register_local_functions(current_scope());
  //  Imports take precedence over variables
  register_imports(current_scope());

  BoxedBlock function_body;

  {
    Store::Write read_write(*store);
    auto& def = read_write.at(def_node.def_handle);

    register_function_parameters(read_write, def.header.name_token, def.header.inputs);
    register_function_parameters(read_write, def.header.name_token, def.header.outputs);

    function_body = std::move(def.body);
  }

  //  Enter function definition.
  FunctionDefState::EnclosingFunctionHelper function_helper(function_state, def_node.def_handle);
  block_new_context(function_body);
  pop_scope();

  {
    //  Reassign transformed function body.
    Store::ReadMut reader(*store);
    auto& def = reader.at(def_node.def_handle);
    def.body = std::move(function_body);
  }

  return &def_node;
}

ClassDefNode* IdentifierClassifier::class_def_node(ClassDefNode& ref) {
  ClassDefState::EnclosingClassHelper class_helper(class_state, ref.handle);

  for (auto& method_node : ref.method_defs) {
    conditional_reset(method_node, method_node->accept(*this));
  }

  for (auto& property : ref.properties) {
    auto& initializer = property.initializer;

    if (initializer) {
      conditional_reset(initializer, initializer->accept(*this));
    }
  }

  return &ref;
}

FunTypeNode* IdentifierClassifier::fun_type_node(FunTypeNode& node) {
  conditional_reset(node.definition, node.definition->accept(*this));
  return &node;
}

TypeAnnotMacro* IdentifierClassifier::type_annot_macro(TypeAnnotMacro& node) {
  conditional_reset(node.annotation, node.annotation->accept(*this));
  return &node;
}

void IdentifierClassifier::transform_root(BoxedRootBlock& block) {
  conditional_reset(block, block->accept(*this));
}

RootBlock* IdentifierClassifier::root_block(RootBlock& block) {
  push_scope(block.scope);

  register_local_functions(current_scope());
  register_imports(current_scope());

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

void IdentifierClassifier::add_error_if_new_identifier(mt::ParseError&& err, const MatlabIdentifier& identifier) {
  if (!added_error_for_identifier(identifier)) {
    add_error(std::move(err));
    mark_error_identifier(identifier);
  }
}

void IdentifierClassifier::add_warning_if_new_identifier(mt::ParseError&& err, const MatlabIdentifier& identifier) {
  if (!added_error_for_identifier(identifier)) {
    warnings.emplace_back(std::move(err));
    mark_error_identifier(identifier);
  }
}

bool IdentifierClassifier::added_error_for_identifier(const MatlabIdentifier& identifier) const {
  return error_identifiers_by_scope.back().count(identifier.full_name()) > 0;
}

void IdentifierClassifier::mark_error_identifier(const MatlabIdentifier& identifier) {
  error_identifiers_by_scope.back().insert(identifier.full_name());
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
                                                                       const MatlabIdentifier& identifier,
                                                                       IdentifierType present_type) {
  auto identifier_str = string_registry->at(identifier.full_name());
  auto type_str = std::string(to_string(present_type));

  std::string msg = "The usage of `" + identifier_str + "` as a variable assignment target ";
  msg += "is inconsistent with its previous usage as a `" + type_str + "`.";

  return ParseError(text, at_token, msg, file_descriptor);
}

ParseError IdentifierClassifier::make_error_variable_referenced_before_assignment(const Token& at_token,
                                                                                  const MatlabIdentifier& identifier) {
  auto identifier_str = string_registry->at(identifier.full_name());

  std::string msg = "The identifier `" + identifier_str + "`, apparently a variable, might be ";
  msg += "referenced before it is explicitly assigned a value.";

  return ParseError(text, at_token, msg, file_descriptor);
}

ParseError IdentifierClassifier::make_error_implicit_variable_initialization(const mt::Token& at_token) {
  return ParseError(text, at_token,
    "Variables cannot be implicitly initialized with subscripts.", file_descriptor);
}

ParseError IdentifierClassifier::make_error_invalid_function_call_expr(const Token& at_token) {
  return ParseError(text, at_token, "Functions cannot be `.` or `{}` referenced.", file_descriptor);
}

ParseError IdentifierClassifier::make_error_function_reference_to_non_function(const Token& at_token,
                                                                               const MatlabIdentifier& identifier,
                                                                               IdentifierType present_type) {
  auto identifier_str = string_registry->at(identifier.full_name());
  auto type_str = std::string(to_string(present_type));

  std::string msg = "The usage of `" + identifier_str + "` as a function reference ";
  msg += "is inconsistent with its previous usage as a `" + type_str + "`.";

  return ParseError(text, at_token, msg, file_descriptor);
}

ParseError IdentifierClassifier::make_error_shadowed_import(const Token& at_token, IdentifierType present_type) {
  auto type_str = std::string(to_string(present_type));
  std::string msg = "Import is shadowed by identifier of type `" + type_str + "`.";
  return ParseError(text, at_token, msg, file_descriptor);
}

ParseError IdentifierClassifier::make_error_pre_declared_qualified_variable(const Token& at_token, const MatlabIdentifier& identifier) {
  auto identifier_str = string_registry->at(identifier.full_name());
  std::string msg = "The declaration of identifier `" + identifier_str + "` must precede its initialization.";
  return ParseError(text, at_token, msg, file_descriptor);
}

}
