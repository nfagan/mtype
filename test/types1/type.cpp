#include "type.hpp"
#include "typing.hpp"
#include <cassert>

namespace mt {

const char* to_string(types::DestructuredTuple::Usage usage) {
  switch (usage) {
    case types::DestructuredTuple::Usage::rvalue:
      return "rvalue";
    case types::DestructuredTuple::Usage::lvalue:
      return "lvalue";
    case types::DestructuredTuple::Usage::definition_inputs:
      return "definition_inputs";
    case types::DestructuredTuple::Usage::definition_outputs:
      return "definition_outputs";
    default:
      assert(false);
      return "";
  }
}

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
    default:
      assert(false && "Unhandled.");
  }
}

std::ostream& operator<<(std::ostream& stream, Type::Tag tag) {
  stream << to_string(tag);
  return stream;
}

#define MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(tg, type, member) \
  case tg: \
    new (&member) type(); \
    break;

void Type::default_construct() noexcept {
  switch (tag) {
    case Tag::null:
      break;
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::variable, types::Variable, variable)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::scalar, types::Scalar, scalar)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::abstraction, types::Abstraction, abstraction)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::union_type, types::Union, union_type)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::tuple, types::Tuple, tuple)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::destructured_tuple, types::DestructuredTuple, destructured_tuple)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::list, types::List, list)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::subscript, types::Subscript, subscript)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::constant_value, types::ConstantValue, constant_value)
  }
}

#undef MT_DEBUG_TYPE_DEFAULT_CTOR_CASE

#define MT_DEBUG_TYPE_MOVE_CTOR_CASE(tg, type, member) \
  case tg: \
    new (&member) type(std::move(other.member)); \
    break;

void Type::move_construct(Type&& other) noexcept {
  switch (tag) {
    case Tag::null:
      break;
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::variable, types::Variable, variable)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::scalar, types::Scalar, scalar)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::abstraction, types::Abstraction, abstraction)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::union_type, types::Union, union_type)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::tuple, types::Tuple, tuple)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::destructured_tuple, types::DestructuredTuple, destructured_tuple)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::list, types::List, list)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::subscript, types::Subscript, subscript)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::constant_value, types::ConstantValue, constant_value)
  }
}

#undef MT_DEBUG_TYPE_MOVE_CTOR_CASE

#define MT_DEBUG_TYPE_COPY_CTOR_CASE(tg, type, member) \
  case tg: \
    new (&member) type(other.member); \
    break;

void Type::copy_construct(const Type& other) {
  switch (tag) {
    case Tag::null:
      break;
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::variable, types::Variable, variable)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::scalar, types::Scalar, scalar)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::abstraction, types::Abstraction, abstraction)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::union_type, types::Union, union_type)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::tuple, types::Tuple, tuple)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::destructured_tuple, types::DestructuredTuple, destructured_tuple)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::list, types::List, list)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::subscript, types::Subscript, subscript)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::constant_value, types::ConstantValue, constant_value)
  }
}

#undef MT_DEBUG_TYPE_COPY_CTOR_CASE

#define MT_DEBUG_TYPE_COPY_ASSIGN_CASE(tg, member) \
  case tg: \
    member = other.member; \
    break;

void Type::copy_assign(const Type& other) {
  switch (tag) {
    case Tag::null:
      break;
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::variable, variable)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::scalar, scalar)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::abstraction, abstraction)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::union_type, union_type)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::tuple, tuple)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::destructured_tuple, destructured_tuple)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::list, list)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::subscript, subscript)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::constant_value, constant_value)
  }
}

#undef MT_DEBUG_TYPE_COPY_ASSIGN_CASE

#define MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(tg, member) \
  case tg: \
    member = std::move(other.member); \
    break;

void Type::move_assign(Type&& other) {
  switch (tag) {
    case Tag::null:
      break;
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::variable, variable)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::scalar, scalar)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::abstraction, abstraction)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::union_type, union_type)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::tuple, tuple)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::destructured_tuple, destructured_tuple)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::list, list)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::subscript, subscript)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::constant_value, constant_value)
  }
}

#undef MT_DEBUG_TYPE_MOVE_ASSIGN_CASE

#define MT_DEBUG_TYPE_DTOR_CASE(tg, member, name) \
  case tg: \
    member.~name(); \
    break;

Type::~Type() {
  switch (tag) {
    case Tag::null:
      break;
    MT_DEBUG_TYPE_DTOR_CASE(Tag::variable, variable, Variable)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::scalar, scalar, Scalar)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::abstraction, abstraction, Abstraction)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::union_type, union_type, Union)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::tuple, tuple, Tuple)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::destructured_tuple, destructured_tuple, DestructuredTuple)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::list, list, List)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::subscript, subscript, Subscript)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::constant_value, constant_value, ConstantValue)
  }
}

#undef MT_DEBUG_TYPE_DTOR_CASE

Type& Type::operator=(const Type& other) {
  conditional_default_construct(other.tag);
  copy_assign(other);
  return *this;
}

Type& Type::operator=(Type&& other) noexcept {
  conditional_default_construct(other.tag);
  move_assign(std::move(other));
  return *this;
}

void Type::conditional_default_construct(Type::Tag other_type) noexcept {
  if (tag != other_type) {
    this->~Type();
    tag = other_type;
    default_construct();
  }
}

}