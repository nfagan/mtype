#include "type.hpp"
#include "type_constraint_gen.hpp"
#include <functional>
#include <cassert>

namespace mt {

int types::Abstraction::HeaderCompare::operator()(const Abstraction& a, const Abstraction& b) const {
  if (a.type != b.type) {
    return a.type < b.type ? -1 : 1;
  }

  if (a.type == Type::binary_operator && a.binary_operator != b.binary_operator) {
    return a.binary_operator < b.binary_operator ? -1 : 1;

  } else if (a.type == Type::unary_operator && a.unary_operator != b.unary_operator) {
    return a.unary_operator < b.unary_operator ? -1 : 1;

  } else if (a.type == Type::subscript_reference && a.subscript_method != b.subscript_method) {
    return a.subscript_method < b.subscript_method ? -1 : 1;

  } else if (a.type == Type::function && a.name != b.name) {
    return a.name < b.name ? -1 : 1;

  } else if (a.type == Type::concatenation && a.concatenation_direction != b.concatenation_direction) {
    return a.concatenation_direction < b.concatenation_direction ? -1 : 1;
  }

  return 0;
}

std::size_t TypeIdentifier::Hash::operator()(const TypeIdentifier& id) const {
  return std::hash<int64_t>{}(id.name);
}

TypeEquationTerm make_term(const Token* source_token, const TypeHandle& term) {
  return TypeEquationTerm(source_token, term);
}

TypeEquation make_eq(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs) {
  return TypeEquation(lhs, rhs);
}

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
    case Tag::scheme:
      return "scheme";
    case Tag::assignment:
      return "assignment";
    case Tag::parameters:
      return "parameters";
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
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::scheme, types::Scheme, scheme)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::assignment, types::Assignment, assignment)
    MT_DEBUG_TYPE_DEFAULT_CTOR_CASE(Tag::parameters, types::Parameters, parameters)
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
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::scheme, types::Scheme, scheme)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::assignment, types::Assignment, assignment)
    MT_DEBUG_TYPE_MOVE_CTOR_CASE(Tag::parameters, types::Parameters, parameters)
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
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::scheme, types::Scheme, scheme)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::assignment, types::Assignment, assignment)
    MT_DEBUG_TYPE_COPY_CTOR_CASE(Tag::parameters, types::Parameters, parameters)
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
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::scheme, scheme)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::assignment, assignment)
    MT_DEBUG_TYPE_COPY_ASSIGN_CASE(Tag::parameters, parameters)
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
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::scheme, scheme)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::assignment, assignment)
    MT_DEBUG_TYPE_MOVE_ASSIGN_CASE(Tag::parameters, parameters)
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
    MT_DEBUG_TYPE_DTOR_CASE(Tag::scheme, scheme, Scheme)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::assignment, assignment, Assignment)
    MT_DEBUG_TYPE_DTOR_CASE(Tag::parameters, parameters, Parameters)
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