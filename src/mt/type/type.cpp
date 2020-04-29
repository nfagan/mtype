#include "type.hpp"
#include "type_representation.hpp"
#include "../token.hpp"
#include <functional>
#include <cassert>

namespace mt {

bool Type::Less::operator()(const Type* a, const Type* b) const noexcept {
  return a->compare(b) == -1;
}

bool Type::Equal::operator()(const Type* a, const Type* b) const noexcept {
  return a->compare(b) == 0;
}

int Type::Compare::operator()(const Type* a, const Type* b) const noexcept {
  return a->compare(b);
}

/*
 * Util
 */

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
    case Tag::application:
      return "application";
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
    case Tag::record:
      return "record";
    case Tag::alias:
      return "alias";
    default:
      assert(false && "Unhandled.");
  }

  assert(false);
  return "null";
}

std::ostream& operator<<(std::ostream& stream, Type::Tag tag) {
  stream << to_string(tag);
  return stream;
}

}