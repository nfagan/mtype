#pragma once

#include "mt/mt.hpp"
#include <cstdint>
#include <iostream>

#define MT_TYPE_VISITOR_METHOD(name, type) \
  virtual void name(type&) {} \
  virtual void name(const type&) {}

namespace mt {

class TypeVisitor;

struct TypeHandle : public detail::Handle<100> {
  friend class TypeVisitor;
  friend class TypeStore;
  using Handle::Handle;
};

struct Type;
struct TypeIdentifier {
  TypeIdentifier() : TypeIdentifier(-1) {
    //
  }

  explicit TypeIdentifier(int64_t name) : name(name) {
    //
  }

  int64_t name;
};

namespace types {
  struct Null {
    //
  };

  struct ConstantValue {
    enum class Type {
      int_value = 0,
      double_value
    };

    ConstantValue() : ConstantValue(int64_t(0)) {
      //
    }
    explicit ConstantValue(int64_t val) : type(Type::int_value), int_value(val) {
      //
    }
    explicit ConstantValue(double val) : type(Type::double_value), double_value(val) {
      //
    }

    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(ConstantValue)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(ConstantValue)

    Type type;

    union {
      int64_t int_value;
      double double_value;
    };
  };

  struct Variable {
    Variable() = default;
    explicit Variable(const TypeIdentifier& id) : identifier(id) {
      //
    }

    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Variable)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Variable)

    TypeIdentifier identifier;
  };

  struct Scalar {
    Scalar() = default;
    explicit Scalar(const TypeIdentifier& id) : identifier(id) {
      //
    }
    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Scalar)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Scalar)

    TypeIdentifier identifier;
    std::vector<TypeHandle> arguments;
  };

  struct Union {
    Union() = default;
    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Union)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Union)

    std::vector<TypeHandle> members;
  };

  struct List {
    enum class Usage : uint8_t {
      lvalue,
      rvalue,
      definition
    };

    List() : usage(Usage::definition) {
      //
    }

    List(Usage usage, const TypeHandle& arg) : usage(usage), pattern{arg} {
      //
    }

    List(Usage usage, std::vector<TypeHandle>&& pattern) : usage(usage), pattern(std::move(pattern)) {
      //
    }

    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(List)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(List)

    int64_t size() const {
      return pattern.size();
    }

    Usage usage;
    std::vector<TypeHandle> pattern;
  };

  struct Subscript {
    struct Sub {
      Sub(SubscriptMethod method, std::vector<TypeHandle>&& args) :
      method(method), arguments(std::move(args)) {
        //
      }

      SubscriptMethod method;
      std::vector<TypeHandle> arguments;
    };

    Subscript(const TypeHandle& arg, std::vector<Sub>&& subs, const TypeHandle& outputs) :
    principal_argument(arg), subscripts(std::move(subs)), outputs(outputs) {
      //
    }

    Subscript() = default;
    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Subscript)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Subscript)

    TypeHandle principal_argument;
    std::vector<Sub> subscripts;
    TypeHandle outputs;
  };

  struct DestructuredTuple {
    enum class Usage : uint8_t {
      rvalue,
      lvalue,
      definition_outputs,
      definition_inputs,
    };

    DestructuredTuple() : usage(Usage::definition_inputs) {
      //
    }
    DestructuredTuple(Usage usage, const TypeHandle& a, const TypeHandle& b) :
    usage(usage), members{a, b} {
      //
    }
    DestructuredTuple(Usage usage, const TypeHandle& a) :
    usage(usage), members{a} {
      //
    }
    DestructuredTuple(Usage usage, std::vector<TypeHandle>&& members) :
    usage(usage), members(std::move(members)) {
      //
    }

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

    static bool mismatching_definition_usages(const DestructuredTuple& a, const DestructuredTuple& b) {
      return (a.is_inputs() && b.is_outputs()) || (a.is_outputs() && b.is_inputs());
    }

    int64_t size() const {
      return members.size();
    }

    Usage usage;
    std::vector<TypeHandle> members;
  };

  struct Tuple {
    Tuple() = default;
    explicit Tuple(const TypeHandle& handle) : members{handle} {
      //
    }
    explicit Tuple(std::vector<TypeHandle>&& members) : members(std::move(members)) {
      //
    }

    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Tuple)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Tuple)

    std::vector<TypeHandle> members;
  };

  struct Abstraction {
    enum class Type : uint8_t {
      unary_operator = 0,
      binary_operator,
      subscript_reference,
      function,
    };

    Abstraction() : type(Type::function) {
      //
    }

    Abstraction(BinaryOperator binary_operator, const TypeHandle& args, const TypeHandle& result) :
    type(Type::binary_operator), binary_operator(binary_operator), inputs{args}, outputs{result} {
      //
    }
    Abstraction(UnaryOperator unary_operator, const TypeHandle& arg, const TypeHandle& result) :
    type(Type::unary_operator), unary_operator(unary_operator), inputs{arg}, outputs{result} {
      //
    }
    Abstraction(SubscriptMethod subscript_method, const TypeHandle& args, const TypeHandle& result) :
    type(Type::subscript_reference), subscript_method(subscript_method), inputs{args}, outputs{result} {
      //
    }
    Abstraction(MatlabIdentifier name, const TypeHandle& inputs, const TypeHandle& outputs) :
    type(Type::function), name(name), inputs(std::move(inputs)), outputs(std::move(outputs)) {
      //
    }

    Abstraction(const Abstraction& other) :
    type(other.type), outputs(other.outputs), inputs(other.inputs) {
      conditional_assign_operator(other);
    }
    Abstraction(Abstraction&& other) noexcept :
    type(other.type), outputs(std::move(other.outputs)), inputs(std::move(other.inputs)) {
      conditional_assign_operator(other);
    }

    Abstraction& operator=(const Abstraction& other) {
      type = other.type;
      outputs = other.outputs;
      inputs = other.inputs;
      conditional_assign_operator(other);
      return *this;
    }

    Abstraction& operator=(Abstraction&& other) noexcept {
      type = other.type;
      outputs = std::move(other.outputs);
      inputs = std::move(other.inputs);
      conditional_assign_operator(other);
      return *this;
    }

    bool is_function() const {
      return type == Type::function;
    }

    bool is_binary_operator() const {
      return type == Type::binary_operator;
    }

  private:
    void conditional_assign_operator(const Abstraction& other) {
      if (other.type == Type::binary_operator) {
        binary_operator = other.binary_operator;

      } else if (other.type == Type::unary_operator) {
        unary_operator = other.unary_operator;

      } else if (other.type == Type::subscript_reference) {
        subscript_method = other.subscript_method;

      } else if (other.type == Type::function) {
        name = other.name;
      }
    }
  public:
    Type type;

    union {
      BinaryOperator binary_operator;
      UnaryOperator unary_operator;
      SubscriptMethod subscript_method;
      MatlabIdentifier name;
    };

    TypeHandle outputs;
    TypeHandle inputs;
  };
}

/*
 * DebugType
 */

#define MT_DEBUG_TYPE_RVALUE_CTOR(type, t, member) \
  explicit Type(type&& v) : tag(t) { \
    new (&member) type(std::move(v)); \
  }

class Type {
public:
  enum class Tag : uint8_t {
    null = 0,
    variable,
    scalar,
    abstraction,
    union_type,
    tuple,
    destructured_tuple,
    list,
    subscript,
    constant_value
  };

  Type() : Type(types::Null{}) {
    //
  }

  explicit Type(types::Null&& null) : tag(Tag::null) {
    //
  }

  MT_DEBUG_TYPE_RVALUE_CTOR(types::Variable, Tag::variable, variable);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Scalar, Tag::scalar, scalar);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Abstraction, Tag::abstraction, abstraction);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Union, Tag::union_type, union_type);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Tuple, Tag::tuple, tuple);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::DestructuredTuple, Tag::destructured_tuple, destructured_tuple);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::List, Tag::list, list);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Subscript, Tag::subscript, subscript);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::ConstantValue, Tag::constant_value, constant_value);

  Type(const Type& other) : tag(other.tag) {
    copy_construct(other);
  }

  Type(Type&& other) noexcept : tag(other.tag) {
    move_construct(std::move(other));
  }

  Type& operator=(const Type& other);
  Type& operator=(Type&& other) noexcept;

  ~Type();

  [[nodiscard]] bool is_variable() const {
    return tag == Tag::variable;
  }

  [[nodiscard]] bool is_destructured_tuple() const {
    return tag == Tag::destructured_tuple;
  }

  [[nodiscard]] bool is_list() const {
    return tag == Tag::list;
  }

private:
  void conditional_default_construct(Tag other_type) noexcept;
  void default_construct() noexcept;
  void move_construct(Type&& other) noexcept;
  void copy_construct(const Type& other);
  void copy_assign(const Type& other);
  void move_assign(Type&& other);

public:
  Tag tag;

  union {
    types::Null null;
    types::Variable variable;
    types::Scalar scalar;
    types::Union union_type;
    types::Abstraction abstraction;
    types::Tuple tuple;
    types::DestructuredTuple destructured_tuple;
    types::List list;
    types::Subscript subscript;
    types::ConstantValue constant_value;
  };
};

const char* to_string(Type::Tag tag);
const char* to_string(types::DestructuredTuple::Usage usage);
std::ostream& operator<<(std::ostream& stream, Type::Tag tag);

}

#undef MT_DEBUG_TYPE_RVALUE_CTOR
#undef MT_TYPE_VISITOR_METHOD