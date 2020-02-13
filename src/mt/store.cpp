#include "store.hpp"
#include <cassert>

namespace mt {

FunctionReference* FunctionRegistry::make_external_reference(int64_t to_identifier, std::shared_ptr<MatlabScope> in_scope) {
  const auto def = nullptr;
  auto ref = std::make_unique<FunctionReference>(to_identifier, def, std::move(in_scope));
  auto ptr = ref.get();
  external_function_references.emplace_back(std::move(ref));
  return ptr;
}

void FunctionRegistry::emplace_local_definition(std::unique_ptr<FunctionDef> function_def) {
  local_function_definitions.emplace_back(std::move(function_def));
}

/*
 * ClassDefStore
 */

ClassDefHandle ClassDefStore::emplace_definition(ClassDef&& def) {
  definitions.emplace_back(std::move(def));
  int64_t index = definitions.size() - 1;

  return ClassDefHandle(index);
}

ClassDef& ClassDefStore::lookup_class(ClassDefHandle& by_handle) {
  assert(by_handle.index >= 0 && by_handle.index < definitions.size() && "Out of bounds index.");
  return definitions[by_handle.index];
}

const ClassDef& ClassDefStore::lookup_class(const ClassDefHandle& by_handle) const {
  assert(by_handle.index >= 0 && by_handle.index < definitions.size() && "Out of bounds index.");
  return definitions[by_handle.index];
}

}
