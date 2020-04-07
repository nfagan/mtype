#include "type_properties.hpp"
#include "type_store.hpp"

namespace mt {

bool TypeProperties::is_concrete_argument(const Type* type) const {
  using Tag = Type::Tag;

  switch (type->tag) {
    case Tag::destructured_tuple:
      return is_concrete_argument(MT_DT_REF(*type));
    case Tag::list:
      return is_concrete_argument(MT_LIST_REF(*type));
    case Tag::abstraction:
      return is_concrete_argument(MT_ABSTR_REF(*type));
    case Tag::scheme:
      return is_concrete_argument(MT_SCHEME_REF(*type));
    case Tag::tuple:
    case Tag::scalar:
    case Tag::class_type:
      return true;
    default:
      return false;
  }
}

bool TypeProperties::are_concrete_arguments(const TypePtrs& args) const {
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

bool TypeProperties::is_concrete_argument(const mt::types::Abstraction&) const {
  return true;
}

bool TypeProperties::is_concrete_argument(const types::List& list) const {
  return are_concrete_arguments(list.pattern);
}

bool TypeProperties::is_concrete_argument(const types::Scheme& scheme) const {
  return is_concrete_argument(scheme.type);
}

}