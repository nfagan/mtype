#include "traversal.hpp"
#include "ast.hpp"
#include "identifier.hpp"
#include "Optional.hpp"
#include <cassert>

namespace mt {

/*
 * TypeIdentifierNamespaceState
 */

void TypeIdentifierNamespaceState::Stack::push(TypeIdentifierNamespaceState& state, const TypeIdentifier& ident) {
  state.push(ident);
}

void TypeIdentifierNamespaceState::Stack::pop(TypeIdentifierNamespaceState& state) {
  state.pop();
}

void TypeIdentifierNamespaceState::push(const TypeIdentifier& ident) {
  components.push_back(ident);
}

void TypeIdentifierNamespaceState::pop() {
  assert(!components.empty());
  components.pop_back();
}

Optional<TypeIdentifier> TypeIdentifierNamespaceState::enclosing_namespace() const {
  return components.empty() ? NullOpt{} : Optional<TypeIdentifier>(components.back());
}

void TypeIdentifierNamespaceState::gather_components(std::vector<int64_t>& into) const {
  for (const auto& ident : components) {
    into.push_back(ident.full_name());
  }
}

bool TypeIdentifierNamespaceState::has_enclosing_namespace() const {
  return !components.empty();
}

/*
 * TypeIdentifierExportState
 */

void TypeIdentifierExportState::Stack::push(TypeIdentifierExportState& state, bool value) {
  if (value) {
    state.push_export();
  } else {
    state.push_non_export();
  }
}

void TypeIdentifierExportState::Stack::pop(TypeIdentifierExportState& state) {
  state.pop_state();
}

void TypeIdentifierExportState::push_export() {
  state.push_back(true);
}
void TypeIdentifierExportState::push_non_export() {
  state.push_back(false);
}
void TypeIdentifierExportState::pop_state() {
  assert(!state.empty());
  state.pop_back();
}
bool TypeIdentifierExportState::is_export() const {
  assert(!state.empty());
  return state.back();
}

/*
 * AssignmentSourceState
 */

void AssignmentSourceState::push_assignment_target_rvalue() {
  state.push_back(true);
}
void AssignmentSourceState::push_non_assignment_target_rvalue() {
  state.push_back(false);
}
void AssignmentSourceState::pop_assignment_target_state() {
  assert(!state.empty() && "No target state to pop.");
  state.pop_back();
}
bool AssignmentSourceState::is_assignment_target_rvalue() const {
  assert(!state.empty() && "No target state.");
  return state.back();
}

/*
 * BooleanState
 */

void BooleanState::Stack::push(BooleanState& s, bool value) {
  s.push(value);
}

void BooleanState::Stack::pop(BooleanState& s) {
  s.pop();
}

void BooleanState::push(bool value) {
  state.push_back(value);
}

void BooleanState::pop() {
  assert(!state.empty());
  state.pop_back();
}

bool BooleanState::value() const {
  assert(!state.empty());
  return state.back();
}

/*
 * ValueCategoryState
 */

bool ValueCategoryState::is_lhs() const {
  assert(!sides.empty() && "Empty sides.");
  return sides.back();
}

void ValueCategoryState::push_lhs() {
  sides.push_back(true);
}

void ValueCategoryState::push_rhs() {
  sides.push_back(false);
}

void ValueCategoryState::pop_side() {
  assert(!sides.empty() && "No side to pop.");
  sides.pop_back();
}

/*
* ClassDefState
*/

void ClassDefState::push_class(const mt::ClassDefHandle& handle, types::Class* type, const MatlabIdentifier& name) {
  class_defs.push_back(handle);
  class_types.push_back(type);
  class_names.push_back(name);
}

void ClassDefState::pop_class() {
  assert(!class_defs.empty() && "No class to pop.");
  class_defs.pop_back();
  class_types.pop_back();
  class_names.pop_back();
}

ClassDefHandle ClassDefState::enclosing_class() const {
  return class_defs.empty() ? ClassDefHandle() : class_defs.back();
}

types::Class* ClassDefState::enclosing_class_type() const {
  return class_types.empty() ? nullptr : class_types.back();
}

MatlabIdentifier ClassDefState::enclosing_class_name() const {
  return class_names.empty() ? MatlabIdentifier() : class_names.back();
}

bool ClassDefState::is_within_class() const {
  return class_defs.empty() ? false : class_defs.back().is_valid();
}

void ClassDefState::push_default_state() {
  push_non_superclass_method_application();
}

void ClassDefState::push_superclass_method_application() {
  is_superclass_method_application.push_back(true);
}

void ClassDefState::push_non_superclass_method_application() {
  is_superclass_method_application.push_back(false);
}

void ClassDefState::pop_superclass_method_application() {
  assert(!is_superclass_method_application.empty() && "No superclass method application side to pop.");
  is_superclass_method_application.pop_back();
}

bool ClassDefState::is_within_superclass_method_application() const {
  return is_superclass_method_application.back();
}

/*
 * FunctionDefState
 */

FunctionDefHandle FunctionDefState::enclosing_function() const {
  return function_defs.empty() ? FunctionDefHandle() : function_defs.back();
}

void FunctionDefState::push_function(const mt::FunctionDefHandle& func) {
  function_defs.push_back(func);
}

void FunctionDefState::pop_function() {
  assert(!function_defs.empty() && "No function to pop.");
  function_defs.pop_back();
}

}
