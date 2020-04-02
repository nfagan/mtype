#include "type.hpp"
#include "type_representation.hpp"
#include <functional>
#include <cassert>

namespace mt {

/*
 * Variable
 */

void types::Variable::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Scalar
 */

void types::Scalar::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Abstraction
 */

void types::Abstraction::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Union
 */

void types::Union::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Tuple
 */

void types::Tuple::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * DestructuredTuple
 */

void types::DestructuredTuple::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * List
 */

void types::List::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Subscript
 */

void types::Subscript::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * ConstantValue
 */

void types::ConstantValue::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Scheme
 */

void types::Scheme::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Assignment
 */

void types::Assignment::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * Parameters
 */

void types::Parameters::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

/*
 * HeaderCompare
 */

int types::Abstraction::HeaderCompare::operator()(const Abstraction& a, const Abstraction& b) const {
  if (a.kind != b.kind) {
    return a.kind < b.kind ? -1 : 1;
  }

  if (a.kind == Kind::binary_operator && a.binary_operator != b.binary_operator) {
    return a.binary_operator < b.binary_operator ? -1 : 1;

  } else if (a.kind == Kind::unary_operator && a.unary_operator != b.unary_operator) {
    return a.unary_operator < b.unary_operator ? -1 : 1;

  } else if (a.kind == Kind::subscript_reference && a.subscript_method != b.subscript_method) {
    return a.subscript_method < b.subscript_method ? -1 : 1;

  } else if (a.kind == Kind::function && a.name != b.name) {
    return a.name < b.name ? -1 : 1;

  } else if (a.kind == Kind::concatenation && a.concatenation_direction != b.concatenation_direction) {
    return a.concatenation_direction < b.concatenation_direction ? -1 : 1;
  }

  return 0;
}

/*
 * TypeIdentifier
 */

std::size_t TypeIdentifier::Hash::operator()(const TypeIdentifier& id) const {
  return std::hash<int64_t>{}(id.name);
}

/*
 * TypeEquationTerm
 */

std::size_t TypeEquationTerm::TypeHash::operator()(const TypeEquationTerm& t) const {
  return std::hash<Type*>{}(t.term);
};

bool TypeEquationTerm::TypeLess::operator()(const TypeEquationTerm& a, const TypeEquationTerm& b) const {
  return a.term < b.term;
}

/*
 * Util
 */

TypeEquationTerm make_term(const Token* source_token, Type* term) {
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

}