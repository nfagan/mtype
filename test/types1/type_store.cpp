#include "type_store.hpp"
#include <cassert>

namespace mt {

void TypeStore::assign(const TypeHandle& at, Type&& type) {
  assert(at.is_valid() && at.get_index() < types.size());
  types[at.get_index()] = std::move(type);
}

const Type& TypeStore::at(const TypeHandle& handle) const {
  assert(handle.is_valid() && handle.get_index() < types.size());
  return types[handle.get_index()];
}

Type& TypeStore::at(const TypeHandle& handle) {
  assert(handle.is_valid() && handle.get_index() < types.size());
  return types[handle.get_index()];
}

}