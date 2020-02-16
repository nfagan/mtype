#include "store.hpp"
#include <cassert>

namespace mt {

/*
 * VariableStore
 */

VariableDefHandle VariableStore::emplace_definition(mt::VariableDef&& def) {
  definitions.emplace_back(std::move(def));
  return VariableDefHandle(definitions.size() - 1);
}

const VariableDef& VariableStore::at(const mt::VariableDefHandle& handle) const {
  return definitions[handle.index];
}

VariableDef& VariableStore::at(const VariableDefHandle& handle) {
  return definitions[handle.index];
}

/*
 * FunctionStore
 */

FunctionDef& FunctionStore::at(const FunctionDefHandle& handle) {
  return definitions[handle.index];
}

const FunctionDef& FunctionStore::at(const FunctionDefHandle& handle) const {
  return definitions[handle.index];
}

const FunctionReference& FunctionStore::at(const FunctionReferenceHandle& handle) const {
  return references[handle.index];
}

FunctionReferenceHandle FunctionStore::make_external_reference(int64_t to_identifier,
                                                               const MatlabScopeHandle& in_scope) {
  return make_local_reference(to_identifier, FunctionDefHandle(), in_scope);
}

FunctionDefHandle FunctionStore::emplace_definition(FunctionDef&& def) {
  definitions.emplace_back(std::move(def));
  return FunctionDefHandle(definitions.size() - 1);
}

FunctionReferenceHandle FunctionStore::make_local_reference(int64_t to_identifier,
                                                            const FunctionDefHandle& with_def,
                                                            const MatlabScopeHandle& in_scope) {
  FunctionReference reference(to_identifier, with_def, in_scope);
  references.emplace_back(reference);
  return FunctionReferenceHandle(references.size() - 1);
}

/*
 * ClassStore
 */

ClassDefHandle ClassStore::make_definition() {
  definitions.emplace_back();
  return ClassDefHandle(definitions.size() - 1);
}

void ClassStore::emplace_definition(const ClassDefHandle& at_handle, ClassDef&& def) {
  definitions[at_handle.index] = std::move(def);
}

ClassDef& ClassStore::at(ClassDefHandle& by_handle) {
  assert(by_handle.index >= 0 && by_handle.index < int64_t(definitions.size()) && "Out of bounds index.");
  return definitions[by_handle.index];
}

const ClassDef& ClassStore::at(const ClassDefHandle& by_handle) const {
  assert(by_handle.index >= 0 && by_handle.index < int64_t(definitions.size()) && "Out of bounds index.");
  return definitions[by_handle.index];
}

/*
 * ScopeStore
 */

MatlabScopeHandle ScopeStore::make_matlab_scope(const mt::MatlabScopeHandle& parent) {
  MatlabScope scope(parent);
  matlab_scopes.emplace_back(std::move(scope));
  return MatlabScopeHandle(matlab_scopes.size() - 1);
}

const MatlabScope& ScopeStore::at(const MatlabScopeHandle& handle) const {
  assert(handle.index >= 0 && handle.index < int64_t(matlab_scopes.size()) && "Out of bounds scope handle.");
  return matlab_scopes[handle.index];
}

MatlabScope& ScopeStore::at(const MatlabScopeHandle& handle) {
  assert(handle.index >= 0 && handle.index < int64_t(matlab_scopes.size()) && "Out of bounds scope handle.");
  return matlab_scopes[handle.index];
}

}
