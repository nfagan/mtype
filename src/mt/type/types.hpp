#pragma once

#include "type.hpp"
#include "components.hpp"
#include "../lang_components.hpp"
#include "../identifier.hpp"
#include "../handles.hpp"
#include "../definitions.hpp"
#include <string_view>

#define MT_USE_APPLICATION (1)

namespace mt {

template <typename T>
class Optional;

}

namespace mt::types {

/*
 * Alias
 */

struct Alias : public Type {
  Alias() : Alias(nullptr) {
    //
  }

  explicit Alias(Type* source) : Type(Type::Tag::alias), source(source) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Alias)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Alias)

  ~Alias() override = default;

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  const Type* alias_source() const override;
  Type* alias_source() override;

  int compare(const Type* b) const noexcept override;

  Type* source;
};

/*
 * Record
 */

struct ConstantValue;

struct Record : public Type {
  struct Field {
    Type* name;
    Type* type;
  };

  using Fields = std::vector<Field>;

  Record() : Type(Type::Tag::record) {
    //
  }

  explicit Record(Fields&& fields) : Type(Type::Tag::record), fields(std::move(fields)) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Record)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Record)

  ~Record() override = default;

  std::size_t bytes() const override;
  int64_t num_fields() const;

  const Field* find_field(const types::ConstantValue& val) const;
  const Field* find_field(const TypeIdentifier& ident) const;
  bool has_field(const types::ConstantValue& val) const;

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

  Fields fields;
};

/*
 * Class
 */

struct Class : public Type {
  Class() : Class(TypeIdentifier(), nullptr) {
    //
  }

  //  No supertypes.
  explicit Class(const TypeIdentifier& name, Type* source) :
  Type(Type::Tag::class_type), name(name), source(source) {
    //
  }

  //  One supertype.
  Class(const TypeIdentifier& name, Type* source, Type* supertype) :
    Type(Type::Tag::class_type), name(name), source(source), supertypes{supertype} {
    //
  }

  Class(const TypeIdentifier& name, Type* source, TypePtrs&& supertypes) :
    Type(Type::Tag::class_type), name(name), source(source), supertypes(std::move(supertypes)) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Class)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Class)

  ~Class() override = default;

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

  TypeIdentifier name;
  Type* source;
  TypePtrs supertypes;
};

/*
 * ConstantValue
 */

struct ConstantValue : public Type {
  enum class Kind {
    int_value = 0,
    double_value,
    char_value,
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

  explicit ConstantValue(const TypeIdentifier& val) :
    Type(Type::Tag::constant_value), kind(Kind::char_value), char_value(val) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(ConstantValue)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(ConstantValue)

  ~ConstantValue() override = default;

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  bool is_char_value() const;

  int compare(const Type* b) const noexcept override;

  Kind kind;

  union {
    int64_t int_value;
    double double_value;
    TypeIdentifier char_value;
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

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Variable)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Variable)

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

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

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  Type* scheme_source() override;
  const Type* scheme_source() const override;

  int compare(const Type* b) const noexcept override;

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

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

  TypeIdentifier identifier;
};

/*
 * Union
 */

struct Union : public Type {
  struct UniqueMembers {
    int64_t count() const;

    TypePtrs members;
    TypePtrs::iterator end;
  };

  Union() : Type(Type::Tag::union_type) {
    //
  }

  explicit Union(TypePtrs&& members) :
  Type(Type::Tag::union_type), members(std::move(members)) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Union)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Union)

  ~Union() override = default;

  std::size_t bytes() const override;
  int64_t size() const;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;
  static UniqueMembers unique_members(const types::Union& a);

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

  std::size_t bytes() const override;
  int64_t size() const;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

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

    bool is_parens() const;
    bool is_brace() const;
    bool is_period() const;

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

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

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

  bool is_lvalue() const;
  bool is_rvalue() const;
  bool is_outputs() const;
  bool is_inputs() const;
  bool is_definition_usage() const;
  bool is_value_usage() const;

  int64_t size() const;
  std::size_t bytes() const override;

  void accept(const TypeToString& to_str, std::stringstream& into) const override;
  Optional<Type*> first_non_destructured_tuple_member() const;

  int compare(const Type* b) const noexcept override;

  static bool is_value_usage(Usage use);
  static bool mismatching_definition_usages(const DestructuredTuple& a, const DestructuredTuple& b);
  static Optional<Type*> type_or_first_non_destructured_tuple_member(Type* in);
  static void flatten(const types::DestructuredTuple& dt, TypePtrs& into,
                      const types::DestructuredTuple* parent = nullptr);

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

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

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

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

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

  Abstraction();
  Abstraction(BinaryOperator binary_operator, Type* args, Type* result);
  Abstraction(UnaryOperator unary_operator, Type* arg, Type* result);
  Abstraction(SubscriptMethod subscript_method, Type* args, Type* result);
  Abstraction(MatlabIdentifier name, Type* inputs, Type* outputs);
  Abstraction(MatlabIdentifier name, const FunctionReferenceHandle& ref_handle, Type* inputs, Type* outputs);
  Abstraction(ConcatenationDirection dir, Type* inputs, Type* outputs);
  Abstraction(Type* inputs, Type* outputs);

  Abstraction(const Abstraction& other);
  Abstraction(Abstraction&& other) noexcept;

  ~Abstraction() override = default;

  Abstraction& operator=(const Abstraction& other);
  Abstraction& operator=(Abstraction&& other) noexcept;

  bool is_function() const;
  bool is_binary_operator() const;
  bool is_unary_operator() const;
  bool is_anonymous() const;
  std::size_t bytes() const override;

  void assign_kind(UnaryOperator op);
  void assign_kind(BinaryOperator op);
  void assign_kind(const MatlabIdentifier& name);
  void assign_kind(const MatlabIdentifier& name, const FunctionReferenceHandle& ref_handle);

  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

  static Abstraction clone(const Abstraction& a, Type* inputs, Type* outputs);

private:
  void conditional_assign_operator(const Abstraction& other);

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
 * Application
 */

struct Application : public Type {
  Application() : Application(nullptr, nullptr, nullptr) {
    //
  }
  Application(Type* abstraction, Type* inputs, Type* outputs) :
  Type(Type::Tag::application), abstraction(abstraction),
  inputs(inputs), outputs(outputs) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Application)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Application)

  ~Application() override = default;

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

  Type* abstraction;
  Type* inputs;
  Type* outputs;
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

  std::size_t bytes() const override;
  void accept(const TypeToString& to_str, std::stringstream& into) const override;

  int compare(const Type* b) const noexcept override;

  TypeIdentifier identifier;
};

}

namespace mt {
  const char* to_string(types::DestructuredTuple::Usage usage);
}

#define MT_DT_REF(a) static_cast<const mt::types::DestructuredTuple&>((a))
#define MT_ABSTR_REF(a) static_cast<const mt::types::Abstraction&>((a))
#define MT_APP_REF(a) static_cast<const mt::types::Application&>((a))
#define MT_VAR_REF(a) static_cast<const mt::types::Variable&>((a))
#define MT_SCHEME_REF(a) static_cast<const mt::types::Scheme&>((a))
#define MT_SCALAR_REF(a) static_cast<const mt::types::Scalar&>((a))
#define MT_TUPLE_REF(a) static_cast<const mt::types::Tuple&>((a))
#define MT_UNION_REF(a) static_cast<const mt::types::Union&>((a))
#define MT_PARAMS_REF(a) static_cast<const mt::types::Parameters&>((a))
#define MT_SUBS_REF(a) static_cast<const mt::types::Subscript&>((a))
#define MT_LIST_REF(a) static_cast<const mt::types::List&>((a))
#define MT_ASSIGN_REF(a) static_cast<const mt::types::Assignment&>((a))
#define MT_CLASS_REF(a) static_cast<const mt::types::Class&>((a))
#define MT_RECORD_REF(a) static_cast<const mt::types::Record&>((a))
#define MT_CONST_VAL_REF(a) static_cast<const mt::types::ConstantValue&>((a))
#define MT_ALIAS_REF(a) static_cast<const mt::types::Alias&>((a))

#define MT_CLASS_PTR(a) static_cast<const mt::types::Class*>((a))
#define MT_SCHEME_PTR(a) static_cast<const mt::types::Scheme*>((a))
#define MT_DT_PTR(a) static_cast<const mt::types::DestructuredTuple*>((a))
#define MT_ALIAS_PTR(a) static_cast<const mt::types::Alias*>((a))
#define MT_ABSTR_PTR(a) static_cast<const mt::types::Abstraction*>((a))
#define MT_APP_PTR(a) static_cast<const mt::types::Application*>((a))

#define MT_DT_MUT_REF(a) static_cast<mt::types::DestructuredTuple&>((a))
#define MT_ABSTR_MUT_REF(a) static_cast<mt::types::Abstraction&>((a))
#define MT_APP_MUT_REF(a) static_cast<mt::types::Application&>((a))
#define MT_VAR_MUT_REF(a) static_cast<mt::types::Variable&>((a))
#define MT_SCHEME_MUT_REF(a) static_cast<mt::types::Scheme&>((a))
#define MT_SCALAR_MUT_REF(a) static_cast<mt::types::Scalar&>((a))
#define MT_TUPLE_MUT_REF(a) static_cast<mt::types::Tuple&>((a))
#define MT_UNION_MUT_REF(a) static_cast<mt::types::Union&>((a))
#define MT_PARAMS_MUT_REF(a) static_cast<mt::types::Parameters&>((a))
#define MT_SUBS_MUT_REF(a) static_cast<mt::types::Subscript&>((a))
#define MT_LIST_MUT_REF(a) static_cast<mt::types::List&>((a))
#define MT_ASSIGN_MUT_REF(a) static_cast<mt::types::Assignment&>((a))
#define MT_CLASS_MUT_REF(a) static_cast<mt::types::Class&>((a))
#define MT_RECORD_MUT_REF(a) static_cast<mt::types::Record&>((a))
#define MT_CONST_VAL_MUT_REF(a) static_cast<mt::types::ConstantValue&>((a))
#define MT_ALIAS_MUT_REF(a) static_cast<mt::types::Alias&>((a))

#define MT_CLASS_MUT_PTR(a) static_cast<mt::types::Class*>((a))
#define MT_SCALAR_MUT_PTR(a) static_cast<mt::types::Scalar*>((a))
#define MT_SCHEME_MUT_PTR(a) static_cast<mt::types::Scheme*>((a))
#define MT_CONST_VAL_MUT_PTR(a) static_cast<mt::types::ConstantValue*>((a))
#define MT_DT_MUT_PTR(a) static_cast<mt::types::DestructuredTuple*>((a))
#define MT_ABSTR_MUT_PTR(a) static_cast<mt::types::Abstraction*>((a))
#define MT_APP_MUT_PTR(a) static_cast<mt::types::Application*>((a))
#define MT_TYPE_MUT_PTR(a) static_cast<Type*>((a))