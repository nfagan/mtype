#include "types.hpp"
#include "type_representation.hpp"
#include <cassert>

namespace mt {

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
  if (val.kind != types::ConstantValue::Kind::char_value) {
    return nullptr;
  } else {
    return find_field(val.char_value);
  }
}

const types::Record::Field* types::Record::find_field(const TypeIdentifier& ident) const {
  using Kind = types::ConstantValue::Kind;

  for (const auto& field : fields) {
    const auto& name = field.name;
    if (!name->is_constant_value()) {
      continue;
    }

    const auto& name_ref = MT_CONST_VAL_REF(*name);
    if (name_ref.kind == Kind::char_value && name_ref.char_value == ident) {
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

std::size_t types::Class::bytes() const {
  return sizeof(Class);
}

/*
 * Variable
 */

void types::Variable::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Variable::bytes() const {
  return sizeof(Variable);
}

/*
 * Scalar
 */

void types::Scalar::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Scalar::bytes() const {
  return sizeof(Scalar);
}

/*
 * Abstraction
 */

types::Abstraction::Abstraction() : Abstraction(MatlabIdentifier(), nullptr, nullptr) {
  //
}

types::Abstraction::Abstraction(BinaryOperator binary_operator, Type* args, Type* result) :
  Type(Type::Tag::abstraction), kind(Kind::binary_operator),
  binary_operator(binary_operator), inputs{args}, outputs{result} {
  //
}

types::Abstraction::Abstraction(UnaryOperator unary_operator, Type* arg, Type* result) :
  Type(Type::Tag::abstraction), kind(Kind::unary_operator),
  unary_operator(unary_operator), inputs{arg}, outputs{result} {
  //
}
types::Abstraction::Abstraction(SubscriptMethod subscript_method, Type* args, Type* result) :
  Type(Type::Tag::abstraction), kind(Kind::subscript_reference),
  subscript_method(subscript_method), inputs{args}, outputs{result} {
  //
}
types::Abstraction::Abstraction(MatlabIdentifier name, Type* inputs, Type* outputs) :
  Type(Type::Tag::abstraction), kind(Kind::function),
  name(name), inputs(inputs), outputs(outputs) {
  //
}
types::Abstraction::Abstraction(MatlabIdentifier name, const FunctionReferenceHandle& ref_handle, Type* inputs, Type* outputs) :
  Type(Type::Tag::abstraction), kind(Kind::function),
  name(name), inputs(inputs), outputs(outputs), ref_handle(ref_handle) {
  //
}
types::Abstraction::Abstraction(ConcatenationDirection dir, Type* inputs, Type* outputs) :
  Type(Type::Tag::abstraction), kind(Kind::concatenation),
  concatenation_direction(dir), inputs(inputs), outputs(outputs) {
  //
}
types::Abstraction::Abstraction(Type* inputs, Type* outputs) :
  Type(Type::Tag::abstraction), kind(Kind::anonymous_function),
  inputs(inputs), outputs(outputs) {
  //
}

types::Abstraction::Abstraction(const Abstraction& other) :
  Type(Type::Tag::abstraction), kind(other.kind),
  inputs(other.inputs), outputs(other.outputs), ref_handle(other.ref_handle) {
  conditional_assign_operator(other);
}

types::Abstraction::Abstraction(Abstraction&& other) noexcept :
Type(Type::Tag::abstraction), kind(other.kind),
inputs(other.inputs), outputs(other.outputs), ref_handle(other.ref_handle) {
  conditional_assign_operator(other);
}

types::Abstraction& types::Abstraction::operator=(const Abstraction& other) {
  kind = other.kind;
  outputs = other.outputs;
  inputs = other.inputs;
  ref_handle = other.ref_handle;
  conditional_assign_operator(other);
  return *this;
}

types::Abstraction& types::Abstraction::operator=(Abstraction&& other) noexcept {
  kind = other.kind;
  outputs = other.outputs;
  inputs = other.inputs;
  ref_handle = other.ref_handle;
  conditional_assign_operator(other);
  return *this;
}

bool types::Abstraction::is_function() const {
  return kind == Kind::function;
}

bool types::Abstraction::is_binary_operator() const {
  return kind == Kind::binary_operator;
}

bool types::Abstraction::is_unary_operator() const {
  return kind == Kind::unary_operator;
}

bool types::Abstraction::is_anonymous() const {
  return kind == Kind::anonymous_function;
}

std::size_t types::Abstraction::bytes() const {
  return sizeof(Abstraction);
}

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

void types::Abstraction::conditional_assign_operator(const Abstraction& other) {
  if (other.kind == Kind::binary_operator) {
    binary_operator = other.binary_operator;

  } else if (other.kind == Kind::unary_operator) {
    unary_operator = other.unary_operator;

  } else if (other.kind == Kind::subscript_reference) {
    subscript_method = other.subscript_method;

  } else if (other.kind == Kind::function) {
    name = other.name;

  } else if (other.kind == Kind::concatenation) {
    concatenation_direction = other.concatenation_direction;
  }
}

types::Abstraction types::Abstraction::clone(const types::Abstraction& a, Type* inputs, Type* outputs) {
  auto b = a;
  b.inputs = inputs;
  b.outputs = outputs;
  return b;
}

/*
 * Union
 */

void types::Union::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Union::bytes() const {
  return sizeof(Union);
}

/*
 * Tuple
 */

void types::Tuple::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Tuple::bytes() const {
  return sizeof(Tuple);
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

bool types::DestructuredTuple::is_lvalue() const {
  return usage == Usage::lvalue;
}

bool types::DestructuredTuple::is_rvalue() const {
  return usage == Usage::rvalue;
}

bool types::DestructuredTuple::is_outputs() const {
  return usage == Usage::definition_outputs;
}

bool types::DestructuredTuple::is_inputs() const {
  return usage == Usage::definition_inputs;
}

bool types::DestructuredTuple::is_definition_usage() const {
  return is_outputs() || is_inputs();
}

bool types::DestructuredTuple::is_value_usage() const {
  return is_lvalue() || is_rvalue();
}

bool types::DestructuredTuple::is_value_usage(Usage use) {
  return use == Usage::rvalue || use == Usage::lvalue;
}

bool types::DestructuredTuple::mismatching_definition_usages(const DestructuredTuple& a, const DestructuredTuple& b) {
  return (a.is_inputs() && b.is_outputs()) || (a.is_outputs() && b.is_inputs());
}

Optional<Type*> types::DestructuredTuple::type_or_first_non_destructured_tuple_member(Type* in) {
  if (in->is_destructured_tuple()) {
    return MT_DT_PTR(in)->first_non_destructured_tuple_member();
  } else {
    return Optional<Type*>(in);
  }
}

void types::DestructuredTuple::flatten(const types::DestructuredTuple& dt, TypePtrs& into,
                                       const types::DestructuredTuple* parent) {
  const int64_t max_num = dt.size();
  const int64_t use_num = parent && parent->is_value_usage() ? std::min(int64_t(1), max_num) : max_num;

  for (int64_t i = 0; i < use_num; i++) {
    const auto& mem = dt.members[i];
    if (mem->is_destructured_tuple()) {
      flatten(MT_DT_REF(*mem), into, &dt);
    } else {
      into.push_back(mem);
    }
  }
}

int64_t types::DestructuredTuple::size() const {
  return members.size();
}

std::size_t types::DestructuredTuple::bytes() const {
  return sizeof(DestructuredTuple);
}

/*
 * List
 */

void types::List::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::List::bytes() const {
  return sizeof(List);
}

int64_t types::List::size() const {
  return pattern.size();
}

/*
 * Subscript
 */

bool types::Subscript::Sub::is_parens() const {
  return method == SubscriptMethod::parens;
}

bool types::Subscript::Sub::is_brace() const {
  return method == SubscriptMethod::brace;
}

bool types::Subscript::Sub::is_period() const {
  return method == SubscriptMethod::period;
}

void types::Subscript::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Subscript::bytes() const {
  return sizeof(Subscript);
}

/*
 * ConstantValue
 */

void types::ConstantValue::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::ConstantValue::bytes() const {
  return sizeof(ConstantValue);
}

bool types::ConstantValue::is_char_value() const {
  return kind == Kind::char_value;
}

/*
 * Scheme
 */

void types::Scheme::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Scheme::bytes() const {
  return sizeof(Scheme);
}

/*
 * Assignment
 */

void types::Assignment::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Assignment::bytes() const {
  return sizeof(Assignment);
}

/*
 * Parameters
 */

void types::Parameters::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

std::size_t types::Parameters::bytes() const {
  return sizeof(Parameters);
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