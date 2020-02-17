#include "store.hpp"
#include <cassert>

namespace mt {

/*
 * Class components
 */

ClassDefHandle Store::make_class_definition() {
  class_definitions.emplace_back();
  return ClassDefHandle(class_definitions.size() - 1);
}

void Store::emplace_definition(const ClassDefHandle& at_handle, ClassDef&& def) {
  class_definitions[at_handle.index] = std::move(def);
}

const ClassDef& Store::at(const ClassDefHandle& handle) const {
  return class_definitions[handle.index];
}

ClassDef& Store::at(const ClassDefHandle& handle) {
  return class_definitions[handle.index];
}

/*
 * Variable components
 */

VariableDefHandle Store::emplace_definition(mt::VariableDef&& def) {
  variable_definitions.emplace_back(std::move(def));
  return VariableDefHandle(variable_definitions.size() - 1);
}

const VariableDef& Store::at(const mt::VariableDefHandle& handle) const {
  return variable_definitions[handle.index];
}

VariableDef& Store::at(const VariableDefHandle& handle) {
  return variable_definitions[handle.index];
}

/*
 * Function components
 */

FunctionDef& Store::at(const FunctionDefHandle& handle) {
  return function_definitions[handle.index];
}

const FunctionDef& Store::at(const FunctionDefHandle& handle) const {
  return function_definitions[handle.index];
}

const FunctionReference& Store::at(const FunctionReferenceHandle& handle) const {
  return function_references[handle.index];
}

FunctionDefHandle Store::emplace_definition(FunctionDef&& def) {
  function_definitions.emplace_back(std::move(def));
  return FunctionDefHandle(function_definitions.size() - 1);
}

FunctionReferenceHandle Store::make_external_reference(int64_t to_identifier,
                                                       const MatlabScopeHandle& in_scope) {
  return make_local_reference(to_identifier, FunctionDefHandle(), in_scope);
}

FunctionReferenceHandle Store::make_local_reference(int64_t to_identifier,
                                                    const FunctionDefHandle& with_def,
                                                    const MatlabScopeHandle& in_scope) {
  FunctionReference reference(to_identifier, with_def, in_scope);
  function_references.emplace_back(reference);
  return FunctionReferenceHandle(function_references.size() - 1);
}

/*
 * Scope components
 */

FunctionReferenceHandle Store::lookup_local_function(const MatlabScopeHandle& in_scope, int64_t name) const {
  MatlabScopeHandle read_handle = in_scope;

  while (read_handle.is_valid()) {
    const auto& scope = at(read_handle);
    const auto it = scope.local_functions.find(name);
    if (it == scope.local_functions.end()) {
      read_handle = scope.parent;
    } else {
      return it->second;
    }
  }

  return FunctionReferenceHandle();
}

MatlabScopeHandle Store::make_matlab_scope(const mt::MatlabScopeHandle& parent) {
  MatlabScope scope(parent);
  matlab_scopes.emplace_back(std::move(scope));
  return MatlabScopeHandle(matlab_scopes.size() - 1);
}

const MatlabScope& Store::at(const MatlabScopeHandle& handle) const {
  assert(handle.index >= 0 && handle.index < int64_t(matlab_scopes.size()) && "Out of bounds scope handle.");
  return matlab_scopes[handle.index];
}

MatlabScope& Store::at(const MatlabScopeHandle& handle) {
  assert(handle.index >= 0 && handle.index < int64_t(matlab_scopes.size()) && "Out of bounds scope handle.");
  return matlab_scopes[handle.index];
}

}
