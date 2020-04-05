#include "type.hpp"
#include "type_representation.hpp"
#include "../token.hpp"
#include <functional>
#include <cassert>

namespace mt {

const char* to_string(Type::Tag tag) {
  using Tag = Type::Tag;

  switch (tag) {
    case Tag::null:
      return "null";
    case Tag::variable:
      return "variable";
    case Tag::scalar:
      return "scalar";
    case Tag::abstraction:
      return "abstraction";
    case Tag::union_type:
      return "union_type";
    case Tag::tuple:
      return "tuple";
    case Tag::destructured_tuple:
      return "destructured_tuple";
    case Tag::list:
      return "list";
    case Tag::subscript:
      return "subscript";
    case Tag::constant_value:
      return "constant_value";
    case Tag::scheme:
      return "scheme";
    case Tag::assignment:
      return "assignment";
    case Tag::parameters:
      return "parameters";
    case Tag::class_type:
      return "class_type";
    default:
      assert(false && "Unhandled.");
  }
}

std::ostream& operator<<(std::ostream& stream, Type::Tag tag) {
  stream << to_string(tag);
  return stream;
}

}