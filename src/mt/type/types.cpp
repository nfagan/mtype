#include "types.hpp"
#include "type_representation.hpp"
#include "type_properties.hpp"
#include "type_visitor.hpp"
#include <cassert>
#include <algorithm>

namespace mt {

#define MT_COMPARE_CHECK_TAG_EARLY_RETURN(b) \
  if (tag != (b)->tag) { \
    return tag < (b)->tag ? -1 : 1; \
  }

#define MT_COMPARE_EARLY_RETURN(a, b) \
  if ((a) != (b)) { \
    return (a) < (b) ? -1 : 1; \
  }

#define MT_COMPARE_TEST0_EARLY_RETURN(a) \
  if ((a) != 0) { \
    return (a); \
  }

namespace {
  inline int compare_impl(const TypePtrs& a, const TypePtrs& b) noexcept {
    const int64_t num_a = a.size();
    const int64_t num_b = b.size();

    MT_COMPARE_EARLY_RETURN(num_a, num_b)

    for (int64_t i = 0; i < num_a; i++) {
      const auto res = a[i]->compare(b[i]);
      MT_COMPARE_TEST0_EARLY_RETURN(res)
    }

    return 0;
  }
}

/*
 * Cast
 */

std::size_t types::Cast::bytes() const {
  return sizeof(Cast);
}

void types::Cast::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Cast::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.cast(*this);
}

void types::Cast::accept(TypeVisitor& vis) {
  vis.cast(*this);
}

void types::Cast::accept_const(TypeVisitor& vis) const {
  vis.cast(*this);
}

int types::Cast::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& cast_b = MT_CAST_REF(*b);

  MT_COMPARE_EARLY_RETURN(strategy, cast_b.strategy)

  auto from_res = from->compare(cast_b.from);
  MT_COMPARE_TEST0_EARLY_RETURN(from_res)

  auto to_res = to->compare(cast_b.to);
  MT_COMPARE_TEST0_EARLY_RETURN(to_res)

  return 0;
}

/*
 * Alias
 */

std::size_t types::Alias::bytes() const {
  return sizeof(Alias);
}

void types::Alias::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Alias::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.alias(*this);
}

void types::Alias::accept(TypeVisitor& vis) {
  vis.alias(*this);
}
void types::Alias::accept_const(TypeVisitor& vis) const {
  vis.alias(*this);
}

namespace {
  template <typename T, typename U>
  U* root_alias_source_impl(U* source) {
    //  Assumes no cycles of course.
    while (source->is_alias()) {
      auto tmp = static_cast<T*>(source);
      source = tmp;
    }
    return source;
  }
}

const Type* types::Alias::alias_source() const {
  return root_alias_source_impl<const types::Alias, const Type>(source);
}

Type* types::Alias::alias_source() {
  return root_alias_source_impl<types::Alias, Type>(source);
}

int types::Alias::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  return source->compare(MT_ALIAS_REF(*b).source);
}

/*
 * Record
 */

void types::Record::accept(const TypeToString& to_str, std::stringstream& into) const {
  return to_str.apply(*this, into);
}

bool types::Record::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.record(*this);
}

void types::Record::accept(TypeVisitor& vis) {
  vis.record(*this);
}

void types::Record::accept_const(TypeVisitor& vis) const {
  vis.record(*this);
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

int types::Record::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& rec_b = MT_RECORD_REF(*b);
  MT_COMPARE_EARLY_RETURN(num_fields(), rec_b.num_fields())

  for (int64_t i = 0; i < num_fields(); i++) {
    const auto name_comp = fields[i].name->compare(rec_b.fields[i].name);
    MT_COMPARE_TEST0_EARLY_RETURN(name_comp)

    const auto type_comp = fields[i].type->compare(rec_b.fields[i].type);
    MT_COMPARE_TEST0_EARLY_RETURN(type_comp)
  }

  return 0;
}

/*
 * Class
 */

void types::Class::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Class::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.class_type(*this);
}

void types::Class::accept(TypeVisitor& vis) {
  vis.class_type(*this);
}

void types::Class::accept_const(TypeVisitor& vis) const {
  vis.class_type(*this);
}

std::size_t types::Class::bytes() const {
  return sizeof(Class);
}

int types::Class::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& cls_b = MT_CLASS_REF(*b);
  MT_COMPARE_EARLY_RETURN(name, cls_b.name)

  const auto source_res = source->compare(cls_b.source);
  MT_COMPARE_TEST0_EARLY_RETURN(source_res)

  const int64_t num_supertypes = supertypes.size();
  const int64_t b_num_supertypes = cls_b.supertypes.size();
  MT_COMPARE_EARLY_RETURN(num_supertypes, b_num_supertypes)

  for (int64_t i = 0; i < num_supertypes; i++) {
    const auto st_res = supertypes[i]->compare(cls_b.supertypes[i]);
    MT_COMPARE_TEST0_EARLY_RETURN(st_res)
  }

  return 0;
}

/*
 * Variable
 */

void types::Variable::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Variable::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.variable(*this);
}

void types::Variable::accept(TypeVisitor& vis) {
  vis.variable(*this);
}

void types::Variable::accept_const(TypeVisitor& vis) const {
  vis.variable(*this);
}

std::size_t types::Variable::bytes() const {
  return sizeof(Variable);
}

int types::Variable::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& vb = MT_VAR_REF(*b);
  MT_COMPARE_EARLY_RETURN(identifier, vb.identifier)
  return 0;
}

/*
 * Scalar
 */

void types::Scalar::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Scalar::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.scalar(*this);
}

void types::Scalar::accept(TypeVisitor& vis) {
  vis.scalar(*this);
}

void types::Scalar::accept_const(TypeVisitor& vis) const {
  vis.scalar(*this);
}

std::size_t types::Scalar::bytes() const {
  return sizeof(Scalar);
}

int types::Scalar::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& scl_b = MT_SCALAR_REF(*b);
  MT_COMPARE_EARLY_RETURN(identifier, scl_b.identifier)
  return 0;
}

/*
 * Application
 */

std::size_t types::Application::bytes() const {
  return sizeof(Application);
}
void types::Application::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Application::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.application(*this);
}

void types::Application::accept(TypeVisitor& vis) {
  vis.application(*this);
}

void types::Application::accept_const(TypeVisitor& vis) const {
  vis.application(*this);
}

int types::Application::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& app_b = MT_APP_REF(*b);

  const auto app_res = abstraction->compare(app_b.abstraction);
  MT_COMPARE_TEST0_EARLY_RETURN(app_res)

  const auto input_res = inputs->compare(app_b.inputs);
  MT_COMPARE_TEST0_EARLY_RETURN(input_res)

  const auto output_res = outputs->compare(app_b.outputs);
  MT_COMPARE_TEST0_EARLY_RETURN(output_res)

  return 0;
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
types::Abstraction::Abstraction(MatlabIdentifier name, const FunctionReferenceHandle& ref_handle,
                                Type* inputs, Type* outputs) :
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

bool types::Abstraction::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.abstraction(*this);
}

void types::Abstraction::accept(TypeVisitor& vis) {
  vis.abstraction(*this);
}

void types::Abstraction::accept_const(TypeVisitor& vis) const {
  vis.abstraction(*this);
}

int types::Abstraction::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& abstr_b = MT_ABSTR_REF(*b);

  HeaderCompare header_comparator{};
  const auto header_res = header_comparator(*this, abstr_b);
  MT_COMPARE_TEST0_EARLY_RETURN(header_res)

  auto input_res = inputs->compare(abstr_b.inputs);
  MT_COMPARE_TEST0_EARLY_RETURN(input_res)

  auto output_res = outputs->compare(abstr_b.outputs);
  MT_COMPARE_TEST0_EARLY_RETURN(output_res)

  return 0;
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

void types::Abstraction::assign_kind(const MatlabIdentifier& nm,
                                     const FunctionReferenceHandle& handle) {
  assign_kind(nm);
  ref_handle = handle;
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

bool types::Union::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.union_type(*this);
}

void types::Union::accept(TypeVisitor& vis) {
  vis.union_type(*this);
}

void types::Union::accept_const(TypeVisitor& vis) const {
  vis.union_type(*this);
}

std::size_t types::Union::bytes() const {
  return sizeof(Union);
}

int64_t types::Union::size() const {
  return members.size();
}

int types::Union::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  return compare_impl(members, MT_UNION_REF(*b).members);
}

int64_t types::Union::UniqueMembers::count() const {
  return end - members.begin();
}

types::Union::UniqueMembers types::Union::unique_members(const types::Union& a) {
  auto members = a.members;
  std::sort(members.begin(), members.end(), Type::Less{});
  auto end = std::unique(members.begin(), members.end(), Type::Equal{});
  return {std::move(members), end};
}

/*
 * Tuple
 */

void types::Tuple::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Tuple::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.tuple(*this);
}

void types::Tuple::accept(TypeVisitor& vis) {
  vis.tuple(*this);
}

void types::Tuple::accept_const(TypeVisitor& vis) const {
  vis.tuple(*this);
}

std::size_t types::Tuple::bytes() const {
  return sizeof(Tuple);
}

int types::Tuple::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& tup = MT_TUPLE_REF(*b);
  return compare_impl(members, tup.members);
}

/*
 * DestructuredTuple
 */

void types::DestructuredTuple::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::DestructuredTuple::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.destructured_tuple(*this);
}

void types::DestructuredTuple::accept(TypeVisitor& vis) {
  vis.destructured_tuple(*this);
}

void types::DestructuredTuple::accept_const(TypeVisitor& vis) const {
  vis.destructured_tuple(*this);
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

bool types::DestructuredTuple::mismatching_definition_usages(const DestructuredTuple& a,
                                                             const DestructuredTuple& b) {
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

int types::DestructuredTuple::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& dt_b = MT_DT_REF(*b);
  MT_COMPARE_EARLY_RETURN(usage, dt_b.usage)
  return compare_impl(members, dt_b.members);
}

/*
 * List
 */

void types::List::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::List::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.list(*this);
}

void types::List::accept(TypeVisitor& vis) {
  vis.list(*this);
}

void types::List::accept_const(TypeVisitor& vis) const {
  vis.list(*this);
}

std::size_t types::List::bytes() const {
  return sizeof(List);
}

int64_t types::List::size() const {
  return pattern.size();
}

int types::List::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  return compare_impl(pattern, MT_LIST_REF(*b).pattern);
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

bool types::Subscript::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.subscript(*this);
}

void types::Subscript::accept(TypeVisitor& vis) {
  vis.subscript(*this);
}

void types::Subscript::accept_const(TypeVisitor& vis) const {
  vis.subscript(*this);
}

std::size_t types::Subscript::bytes() const {
  return sizeof(Subscript);
}

int types::Subscript::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& sub_b = MT_SUBS_REF(*b);

  const auto arg0_res = principal_argument->compare(sub_b.principal_argument);
  MT_COMPARE_TEST0_EARLY_RETURN(arg0_res)

  const auto out_res = outputs->compare(sub_b.outputs);
  MT_COMPARE_TEST0_EARLY_RETURN(out_res)

  const int64_t num_subs = subscripts.size();
  const int64_t num_subs_b = sub_b.subscripts.size();
  MT_COMPARE_EARLY_RETURN(num_subs, num_subs_b)

  for (int64_t i = 0; i < num_subs; i++) {
    const auto& subs_a = subscripts[i];
    const auto& subs_b = sub_b.subscripts[i];
    MT_COMPARE_EARLY_RETURN(subs_a.method, subs_b.method)

    auto res = compare_impl(subs_a.arguments, subs_b.arguments);
    MT_COMPARE_TEST0_EARLY_RETURN(res)
  }

  return 0;
}

/*
 * ConstantValue
 */

void types::ConstantValue::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::ConstantValue::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.constant_value(*this);
}

void types::ConstantValue::accept(TypeVisitor& vis) {
  vis.constant_value(*this);
}

void types::ConstantValue::accept_const(TypeVisitor& vis) const {
  vis.constant_value(*this);
}

std::size_t types::ConstantValue::bytes() const {
  return sizeof(ConstantValue);
}

bool types::ConstantValue::is_char_value() const {
  return kind == Kind::char_value;
}

int types::ConstantValue::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& cv_b = MT_CONST_VAL_REF(*b);
  MT_COMPARE_EARLY_RETURN(kind, cv_b.kind)

  switch (kind) {
    case Kind::int_value:
      MT_COMPARE_EARLY_RETURN(int_value, cv_b.int_value)
      break;
    case Kind::double_value:
      MT_COMPARE_EARLY_RETURN(double_value, cv_b.double_value)
      break;
    case Kind::char_value:
      MT_COMPARE_EARLY_RETURN(char_value, cv_b.char_value)
      break;
  }

  return 0;
}

/*
 * Scheme
 */

void types::Scheme::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Scheme::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.scheme(*this);
}

void types::Scheme::accept(TypeVisitor& vis) {
  vis.scheme(*this);
}

void types::Scheme::accept_const(TypeVisitor& vis) const {
  vis.scheme(*this);
}

std::size_t types::Scheme::bytes() const {
  return sizeof(Scheme);
}

Type* types::Scheme::scheme_source() {
  return type;
}

const Type* types::Scheme::scheme_source() const {
  return type;
}

int types::Scheme::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& scheme_b = MT_SCHEME_REF(*b);

  const auto type_res = type->compare(scheme_b.type);
  MT_COMPARE_TEST0_EARLY_RETURN(type_res)

  const int64_t num_params = parameters.size();
  const int64_t num_params_b = scheme_b.parameters.size();
  MT_COMPARE_EARLY_RETURN(num_params, num_params_b)

  for (int64_t i = 0; i < num_params; i++) {
    const auto param_res = parameters[i]->compare(scheme_b.parameters[i]);
    MT_COMPARE_TEST0_EARLY_RETURN(param_res)
  }

  return 0;
}

/*
 * Assignment
 */

void types::Assignment::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Assignment::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.assignment(*this);
}

void types::Assignment::accept(TypeVisitor& vis) {
  vis.assignment(*this);
}

void types::Assignment::accept_const(TypeVisitor& vis) const {
  vis.assignment(*this);
}

std::size_t types::Assignment::bytes() const {
  return sizeof(Assignment);
}

int types::Assignment::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& assign_b = MT_ASSIGN_REF(*b);

  const auto lhs_res = lhs->compare(assign_b.lhs);
  MT_COMPARE_TEST0_EARLY_RETURN(lhs_res)

  const auto rhs_res = rhs->compare(assign_b.rhs);
  MT_COMPARE_TEST0_EARLY_RETURN(rhs_res)
  return 0;
}

/*
 * Parameters
 */

void types::Parameters::accept(const TypeToString& to_str, std::stringstream& into) const {
  to_str.apply(*this, into);
}

bool types::Parameters::accept(const IsFullyConcrete& is_fully_concrete) const {
  return is_fully_concrete.parameters(*this);
}

void types::Parameters::accept(TypeVisitor& vis) {
  vis.parameters(*this);
}

void types::Parameters::accept_const(TypeVisitor& vis) const {
  vis.parameters(*this);
}

std::size_t types::Parameters::bytes() const {
  return sizeof(Parameters);
}

int types::Parameters::compare(const Type* b) const noexcept {
  MT_COMPARE_CHECK_TAG_EARLY_RETURN(b)
  const auto& params_b = MT_PARAMS_REF(*b);
  MT_COMPARE_EARLY_RETURN(identifier, params_b.identifier)
  return 0;
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