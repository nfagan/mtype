#include "types.hpp"
#include "type_representation.hpp"
#include <cassert>

namespace mt {

/*
 * TypedFunctionReference
 */

std::size_t TypedFunctionReference::NameHash::operator()(const TypedFunctionReference& a) const {
  return MatlabIdentifier::Hash{}(a.ref.name);
}

/*
 * Record
 */

void types::Record::accept(const TypeToString& to_str, std::stringstream& into) const {
  return to_str.apply(*this, into);
}

std::size_t types::Record::bytes() const {
  return sizeof(types::Record);
}

int64_t types::Record::num_fields() const {
  return fields.size();
}

const types::Record::Field* types::Record::find_field(const types::ConstantValue& val) const {
  using Kind = types::ConstantValue::Kind;

  if (val.kind != Kind::char_value) {
    return nullptr;
  }

  for (const auto& field : fields) {
    const auto& name = field.name;
    if (!name->is_constant_value()) {
      continue;
    }

    const auto& name_ref = MT_CONST_VAL_REF(*name);
    if (name_ref.kind == Kind::char_value && name_ref.char_value == val.char_value) {
      return &field;
    }
  }

  return nullptr;
}

bool types::Record::has_field(const types::ConstantValue& val) const {
  return find_field(val) != nullptr;
}

/*
 * Class
 */

void types::Class::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

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

void types::Abstraction::assign_kind(UnaryOperator op) {
  kind = Kind::unary_operator;
  unary_operator = op;
}

void types::Abstraction::assign_kind(BinaryOperator op) {
  kind = Kind::binary_operator;
  binary_operator = op;
}

void types::Abstraction::assign_kind(const MatlabIdentifier& ident) {
  kind = Kind::function;
  name = ident;
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

Optional<Type*> types::DestructuredTuple::first_non_destructured_tuple_member() const {
  for (const auto& m : members) {
    if (m->is_destructured_tuple()) {
      return MT_DT_REF(*m).first_non_destructured_tuple_member();
    } else {
      return Optional<Type*>(m);
    }
  }

  return NullOpt{};
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

bool types::Abstraction::HeaderLess::operator()(const Abstraction& a, const Abstraction& b) const {
  return HeaderCompare{}(a, b) == -1;
}

/*
 * Util
 */

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

}