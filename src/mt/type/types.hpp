#pragma once

#include "type.hpp"
#include "components.hpp"
#include "../lang_components.hpp"
#include "../identifier.hpp"
#include "../handles.hpp"
#include "../definitions.hpp"
#include "../Optional.hpp"

namespace mt {
struct TypedFunctionReference {
  struct NameHash {
    std::size_t operator()(const TypedFunctionReference& a) const;
  };

  TypedFunctionReference() : ref(MatlabIdentifier(), FunctionDefHandle(), nullptr), type(nullptr) {
    //
  }

  FunctionReference ref;
  Type* type;
};
}

namespace mt::types {

/*
 * Class
 */

struct Class : public Type {
  Class() : Class(nullptr) {
    //
  }

  //  No supertypes.
  explicit Class(Type* source) : Type(Type::Tag::class_type), source(source) {
    //
  }

  //  One supertype.
  Class(Type* source, Type* supertype) :
    Type(Type::Tag::class_type), source(source), supertypes{supertype} {
    //
  }

  Class(Type* source, TypePtrs&& supertypes) :
    Type(Type::Tag::class_type), source(source), supertypes(std::move(supertypes)) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Class)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Class)

  ~Class() override = default;

  std::size_t bytes() const override {
    return sizeof(Class);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  Type* source;
  TypePtrs supertypes;
};

/*
 * ConstantValue
 */

struct ConstantValue : public Type {
  enum class Kind {
    int_value = 0,
    double_value
  };

  ConstantValue() : ConstantValue(int64_t(0)) {
    //
  }

  explicit ConstantValue(int64_t val) :
    Type(Type::Tag::constant_value), kind(Kind::int_value), int_value(val) {
    //
  }

  explicit ConstantValue(double val) :
    Type(Type::Tag::constant_value), kind(Kind::double_value), double_value(val) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(ConstantValue)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(ConstantValue)

  ~ConstantValue() override = default;

  std::size_t bytes() const override {
    return sizeof(ConstantValue);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  Kind kind;

  union {
    int64_t int_value;
    double double_value;
  };
};

/*
 * Variable
 */

struct Variable : public Type {
  Variable() : Type(Type::Tag::variable) {
    //
  }
  explicit Variable(const TypeIdentifier& id) : Type(Type::Tag::variable), identifier(id) {
    //
  }
  ~Variable() override = default;

  std::size_t bytes() const override {
    return sizeof(Variable);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Variable)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Variable)

  TypeIdentifier identifier;
};

/*
 * Scheme
 */

struct Scheme : public Type {
  Scheme() : Type(Type::Tag::scheme), type(nullptr) {
    //
  }

  Scheme(Type* type, TypePtrs&& params) :
    Type(Type::Tag::scheme), type(type), parameters(std::move(params)) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Scheme)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Scheme)

  ~Scheme() override = default;

  std::size_t bytes() const override {
    return sizeof(Scheme);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  Type* type;
  TypePtrs parameters;
  std::vector<TypeEquation> constraints;
};

/*
 * Scalar
 */

struct Scalar : public Type {
  Scalar() : Type(Type::Tag::scalar) {
    //
  }
  explicit Scalar(const TypeIdentifier& id) : Type(Type::Tag::scalar), identifier(id) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Scalar)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Scalar)

  ~Scalar() override = default;

  std::size_t bytes() const override {
    return sizeof(Scalar);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  TypeIdentifier identifier;
};

/*
 * Union
 */

struct Union : public Type {
  Union() : Type(Type::Tag::union_type) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Union)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Union)

  ~Union() override = default;

  std::size_t bytes() const override {
    return sizeof(Union);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  TypePtrs members;
};

/*
 * List
 */

struct List : public Type {
  List() : Type(Type::Tag::list) {
    //
  }

  explicit List(Type* arg) : Type(Type::Tag::list), pattern{arg} {
    //
  }

  explicit List(TypePtrs&& pattern) : Type(Type::Tag::list), pattern(std::move(pattern)) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(List)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(List)

  ~List() override = default;

  std::size_t bytes() const override {
    return sizeof(List);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int64_t size() const {
    return pattern.size();
  }

  TypePtrs pattern;
};

/*
 * Subscript
 */

struct Subscript : public Type {
  struct Sub {
    Sub(SubscriptMethod method, TypePtrs&& args) :
      method(method), arguments(std::move(args)) {
      //
    }

    SubscriptMethod method;
    TypePtrs arguments;
  };

  Subscript(Type* arg, std::vector<Sub>&& subs, Type* outputs) :
    Type(Type::Tag::subscript), principal_argument(arg), subscripts(std::move(subs)), outputs(outputs) {
    //
  }

  Subscript() : Type(Type::Tag::subscript), principal_argument(nullptr), outputs(nullptr) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Subscript)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Subscript)

  ~Subscript() override = default;

  std::size_t bytes() const override {
    return sizeof(Subscript);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  Type* principal_argument;
  std::vector<Sub> subscripts;
  Type* outputs;
};

/*
 * DestructuredTuple
 */

struct DestructuredTuple : public Type {
  enum class Usage : uint8_t {
    rvalue,
    lvalue,
    definition_outputs,
    definition_inputs,
  };

  DestructuredTuple() : Type(Type::Tag::destructured_tuple), usage(Usage::definition_inputs) {
    //
  }
  DestructuredTuple(Usage usage, Type* a, Type* b) :
    Type(Type::Tag::destructured_tuple), usage(usage), members{a, b} {
    //
  }
  DestructuredTuple(Usage usage, Type* a) :
    Type(Type::Tag::destructured_tuple), usage(usage), members{a} {
    //
  }
  DestructuredTuple(Usage usage, TypePtrs&& members) :
    Type(Type::Tag::destructured_tuple), usage(usage), members(std::move(members)) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(DestructuredTuple)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(DestructuredTuple)

  ~DestructuredTuple() override = default;

  bool is_lvalue() const {
    return usage == Usage::lvalue;
  }

  bool is_rvalue() const {
    return usage == Usage::rvalue;
  }

  bool is_outputs() const {
    return usage == Usage::definition_outputs;
  }
  bool is_inputs() const {
    return usage == Usage::definition_inputs;
  }

  bool is_definition_usage() const {
    return is_outputs() || is_inputs();
  }

  bool is_value_usage() const {
    return is_lvalue() || is_rvalue();
  }

  static bool is_value_usage(Usage use) {
    return use == Usage::rvalue || use == Usage::lvalue;
  }

  static bool mismatching_definition_usages(const DestructuredTuple& a, const DestructuredTuple& b) {
    return (a.is_inputs() && b.is_outputs()) || (a.is_outputs() && b.is_inputs());
  }

  int64_t size() const {
    return members.size();
  }

  std::size_t bytes() const override {
    return sizeof(DestructuredTuple);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  Optional<Type*> first_non_destructured_tuple_member() const;

  Usage usage;
  TypePtrs members;
};

/*
 * Tuple
 */

struct Tuple : public Type {
  Tuple() : Type(Type::Tag::tuple) {
    //
  }

  explicit Tuple(Type* handle) : Type(Type::Tag::tuple), members{handle} {
    //
  }
  explicit Tuple(TypePtrs&& members) : Type(Type::Tag::tuple), members(std::move(members)) {
    //
  }

  ~Tuple() override = default;

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Tuple)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Tuple)

  std::size_t bytes() const override {
    return sizeof(Tuple);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  TypePtrs members;
};

/*
 * Assignment
 */

struct Assignment : public Type {
  Assignment() : Type(Type::Tag::assignment), lhs(nullptr), rhs(nullptr) {
    //
  }
  Assignment(Type* lhs, Type* rhs) : Type(Type::Tag::assignment), lhs(lhs), rhs(rhs) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Assignment)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Assignment)

  ~Assignment() override = default;

  std::size_t bytes() const override {
    return sizeof(Assignment);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  Type* lhs;
  Type* rhs;
};

/*
 * Abstraction
 */

struct Abstraction : public mt::Type {
  struct HeaderCompare {
    int operator()(const Abstraction& a, const Abstraction& b) const;
  };
  struct HeaderLess {
    bool operator()(const Abstraction& a, const Abstraction& b) const;
  };

  enum class Kind : uint8_t {
    unary_operator = 0,
    binary_operator,
    subscript_reference,
    function,
    concatenation,
    anonymous_function
  };

  Abstraction() : Abstraction(MatlabIdentifier(), nullptr, nullptr) {
    //
  }

  Abstraction(BinaryOperator binary_operator, Type* args, Type* result) :
    Type(Type::Tag::abstraction), kind(Kind::binary_operator),
    binary_operator(binary_operator), inputs{args}, outputs{result} {
    //
  }

  Abstraction(UnaryOperator unary_operator, Type* arg, Type* result) :
    Type(Type::Tag::abstraction), kind(Kind::unary_operator),
    unary_operator(unary_operator), inputs{arg}, outputs{result} {
    //
  }
  Abstraction(SubscriptMethod subscript_method, Type* args, Type* result) :
    Type(Type::Tag::abstraction), kind(Kind::subscript_reference),
    subscript_method(subscript_method), inputs{args}, outputs{result} {
    //
  }
  Abstraction(MatlabIdentifier name, Type* inputs, Type* outputs) :
    Type(Type::Tag::abstraction), kind(Kind::function),
    name(name), inputs(inputs), outputs(outputs) {
    //
  }
  Abstraction(MatlabIdentifier name, const FunctionReferenceHandle& ref_handle, Type* inputs, Type* outputs) :
    Type(Type::Tag::abstraction), kind(Kind::function),
    name(name), inputs(inputs), outputs(outputs), ref_handle(ref_handle) {
    //
  }
  Abstraction(ConcatenationDirection dir, Type* inputs, Type* outputs) :
    Type(Type::Tag::abstraction), kind(Kind::concatenation),
    concatenation_direction(dir), inputs(inputs), outputs(outputs) {
    //
  }
  Abstraction(Type* inputs, Type* outputs) :
    Type(Type::Tag::abstraction), kind(Kind::anonymous_function),
    inputs(inputs), outputs(outputs) {
    //
  }

  Abstraction(const Abstraction& other) :
    Type(Type::Tag::abstraction), kind(other.kind),
    inputs(other.inputs), outputs(other.outputs), ref_handle(other.ref_handle) {
    conditional_assign_operator(other);
  }

  Abstraction(Abstraction&& other) noexcept :
    Type(Type::Tag::abstraction), kind(other.kind),
    inputs(other.inputs), outputs(other.outputs), ref_handle(other.ref_handle) {
    conditional_assign_operator(other);
  }

  ~Abstraction() override = default;

  Abstraction& operator=(const Abstraction& other) {
    kind = other.kind;
    outputs = other.outputs;
    inputs = other.inputs;
    ref_handle = other.ref_handle;
    conditional_assign_operator(other);
    return *this;
  }

  Abstraction& operator=(Abstraction&& other) noexcept {
    kind = other.kind;
    outputs = other.outputs;
    inputs = other.inputs;
    ref_handle = other.ref_handle;
    conditional_assign_operator(other);
    return *this;
  }

  bool is_function() const {
    return kind == Kind::function;
  }

  bool is_binary_operator() const {
    return kind == Kind::binary_operator;
  }

  bool is_unary_operator() const {
    return kind == Kind::unary_operator;
  }

  bool is_anonymous() const {
    return kind == Kind::anonymous_function;
  }

  std::size_t bytes() const override {
    return sizeof(Abstraction);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  static Abstraction clone(const Abstraction& a, Type* inputs, Type* outputs) {
    auto b = a;
    b.inputs = inputs;
    b.outputs = outputs;
    return b;
  }

private:
  void conditional_assign_operator(const Abstraction& other) {
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
public:
  Kind kind;

  union {
    BinaryOperator binary_operator;
    UnaryOperator unary_operator;
    SubscriptMethod subscript_method;
    MatlabIdentifier name;
    ConcatenationDirection concatenation_direction;
  };

  Type* inputs;
  Type* outputs;
  FunctionReferenceHandle ref_handle;
};

/*
 * Parameters
 */

struct Parameters : public Type {
  Parameters() : Type(Type::Tag::parameters) {
    //
  }

  explicit Parameters(const TypeIdentifier& id) : Type(Type::Tag::parameters), identifier(id) {
    //
  }

  ~Parameters() override = default;

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Parameters)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Parameters)

  std::size_t bytes() const override {
    return sizeof(Parameters);
  }

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  TypeIdentifier identifier;
};

}

namespace mt {
  const char* to_string(types::DestructuredTuple::Usage usage);
}

#define MT_DT_REF(a) static_cast<const mt::types::DestructuredTuple&>((a))
#define MT_ABSTR_REF(a) static_cast<const mt::types::Abstraction&>((a))
#define MT_VAR_REF(a) static_cast<const mt::types::Variable&>((a))
#define MT_SCHEME_REF(a) static_cast<const mt::types::Scheme&>((a))
#define MT_SCALAR_REF(a) static_cast<const mt::types::Scalar&>((a))
#define MT_TUPLE_REF(a) static_cast<const mt::types::Tuple&>((a))
#define MT_PARAMS_REF(a) static_cast<const mt::types::Parameters&>((a))
#define MT_SUBS_REF(a) static_cast<const mt::types::Subscript&>((a))
#define MT_LIST_REF(a) static_cast<const mt::types::List&>((a))
#define MT_ASSIGN_REF(a) static_cast<const mt::types::Assignment&>((a))
#define MT_CLASS_REF(a) static_cast<const mt::types::Class&>((a))

#define MT_CLASS_PTR(a) static_cast<const mt::types::Class*>((a))

#define MT_DT_MUT_REF(a) static_cast<mt::types::DestructuredTuple&>((a))
#define MT_ABSTR_MUT_REF(a) static_cast<mt::types::Abstraction&>((a))
#define MT_VAR_MUT_REF(a) static_cast<mt::types::Variable&>((a))
#define MT_SCHEME_MUT_REF(a) static_cast<mt::types::Scheme&>((a))
#define MT_SCALAR_MUT_REF(a) static_cast<mt::types::Scalar&>((a))
#define MT_TUPLE_MUT_REF(a) static_cast<mt::types::Tuple&>((a))
#define MT_PARAMS_MUT_REF(a) static_cast<mt::types::Parameters&>((a))
#define MT_SUBS_MUT_REF(a) static_cast<mt::types::Subscript&>((a))
#define MT_LIST_MUT_REF(a) static_cast<mt::types::List&>((a))
#define MT_ASSIGN_MUT_REF(a) static_cast<mt::types::Assignment&>((a))
#define MT_CLASS_MUT_REF(a) static_cast<mt::types::Class&>((a))

#define MT_CLASS_MUT_PTR(a) static_cast<mt::types::Class*>((a))