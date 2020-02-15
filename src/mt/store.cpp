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
                                                               BoxedMatlabScope in_scope) {
  return make_local_reference(to_identifier, FunctionDefHandle(), std::move(in_scope));
}

FunctionDefHandle FunctionStore::emplace_definition(FunctionDef&& def) {
  definitions.emplace_back(std::move(def));
  return FunctionDefHandle(definitions.size() - 1);
}

FunctionReferenceHandle FunctionStore::make_local_reference(int64_t to_identifier,
                                                            FunctionDefHandle with_def,
                                                            BoxedMatlabScope in_scope) {
  FunctionReference reference(to_identifier, with_def, std::move(in_scope));
  references.emplace_back(reference);
  return FunctionReferenceHandle(references.size() - 1);
}

/*
 * ClassStore
 */

ClassDefHandle ClassStore::emplace_definition(ClassDef&& def) {
  definitions.emplace_back(std::move(def));
  return ClassDefHandle(definitions.size() - 1);
}

ClassDef& ClassStore::at(ClassDefHandle& by_handle) {
  assert(by_handle.index >= 0 && by_handle.index < int64_t(definitions.size()) && "Out of bounds index.");
  return definitions[by_handle.index];
}

const ClassDef& ClassStore::at(const ClassDefHandle& by_handle) const {
  assert(by_handle.index >= 0 && by_handle.index < int64_t(definitions.size()) && "Out of bounds index.");
  return definitions[by_handle.index];
}

}
