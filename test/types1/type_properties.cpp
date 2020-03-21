#include "type_properties.hpp"
#include "type_store.hpp"

namespace mt {

bool TypeProperties::is_concrete_argument(const TypeHandle& handle) const {
  using Tag = Type::Tag;
  const auto& type = store.at(handle);

  switch (type.tag) {
    case Tag::destructured_tuple:
      return is_concrete_argument(type.destructured_tuple);
    case Tag::list:
      return is_concrete_argument(type.list);
    case Tag::abstraction:
      return is_concrete_argument(type.abstraction);
    case Tag::tuple:
    case Tag::scalar:
      return true;
    default:
      return false;
  }
}

bool TypeProperties::are_concrete_arguments(const std::vector<TypeHandle>& args) const {
  for (const auto& arg : args) {
    if (!is_concrete_argument(arg)) {
      return false;
    }
  }
  return true;
}

bool TypeProperties::is_concrete_argument(const types::DestructuredTuple& tup) const {
  return are_concrete_arguments(tup.members);
}

bool TypeProperties::is_concrete_argument(const mt::types::Abstraction& abstr) const {
  return true;
}

bool TypeProperties::is_concrete_argument(const types::List& list) const {
  return are_concrete_arguments(list.pattern);
}

}