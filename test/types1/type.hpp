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
  using Handle::Handle;
};

struct DebugType;
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
    List() = default;
    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(List)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(List)

    std::vector<TypeHandle> pattern;
  };

  struct Tuple {
    enum class Structuring {
      structured,
      destructured
    };

    Tuple() : structuring(Structuring::structured) {
      //
    }
    Tuple(Structuring dir, const TypeHandle& a, const TypeHandle& b) : structuring(dir), members{a, b} {
      //
    }
    Tuple(Structuring dir, const TypeHandle& a) : structuring(dir), members{a} {
      //
    }
    Tuple(Structuring dir, std::vector<TypeHandle>&& members) : structuring(dir), members(std::move(members)) {
      //
    }

    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Tuple)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Tuple)

    Structuring structuring;
    std::vector<TypeHandle> members;
  };

  struct Abstraction {
#if 0
    struct Less {
      bool operator()(const Abstraction& a, const Abstraction& b) const;
    };
#endif

    enum class Type : uint8_t {
      unary_operator = 0,
      binary_operator,
      function,
    };

    Abstraction() : type(Type::function) {
      //
    }

    Abstraction(BinaryOperator binary_operator, const TypeHandle& args, const TypeHandle& result) :
    type(Type::binary_operator),
    binary_operator(binary_operator),
    inputs{args},
    outputs{result} {
      //
    }
    Abstraction(UnaryOperator unary_operator, const TypeHandle& arg, const TypeHandle& result) :
    type(Type::unary_operator),
    unary_operator(unary_operator),
    inputs{arg},
    outputs{result} {
      //
    }
    Abstraction(const TypeHandle& inputs, const TypeHandle& outputs) :
    type(Type::function), inputs(std::move(inputs)), outputs(std::move(outputs)) {
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

#if 0
    bool operator==(const Abstraction& other) const;
#endif

  private:
    void conditional_assign_operator(const Abstraction& other) {
      if (other.type == Type::binary_operator) {
        binary_operator = other.binary_operator;
      } else if (other.type == Type::unary_operator) {
        unary_operator = other.unary_operator;
      }
    }
  public:
    Type type;

    union {
      BinaryOperator binary_operator;
      UnaryOperator unary_operator;
    };

    TypeHandle outputs;
    TypeHandle inputs;
  };
}

/*
 * DebugType
 */

#define MT_DEBUG_TYPE_RVALUE_CTOR(type, t, member) \
  explicit DebugType(type&& v) : tag(t) { \
    new (&member) type(std::move(v)); \
  }

class DebugType {
public:
  enum class Tag {
    null,
    variable,
    scalar,
    abstraction,
    union_type,
    tuple,
    list
  };

  DebugType() : DebugType(types::Null{}) {
    //
  }

  explicit DebugType(types::Null&& null) : tag(Tag::null) {
    //
  }

  MT_DEBUG_TYPE_RVALUE_CTOR(types::Variable, Tag::variable, variable);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Scalar, Tag::scalar, scalar);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Abstraction, Tag::abstraction, abstraction);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Union, Tag::union_type, union_type);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::Tuple, Tag::tuple, tuple);
  MT_DEBUG_TYPE_RVALUE_CTOR(types::List, Tag::list, list);

  DebugType(const DebugType& other) : tag(other.tag) {
    copy_construct(other);
  }

  DebugType(DebugType&& other) noexcept : tag(other.tag) {
    move_construct(std::move(other));
  }

  DebugType& operator=(const DebugType& other);
  DebugType& operator=(DebugType&& other) noexcept;

  ~DebugType();

  [[nodiscard]] bool is_variable() const {
    return tag == Tag::variable;
  }

private:
  void conditional_default_construct(Tag other_type) noexcept;
  void default_construct() noexcept;
  void move_construct(DebugType&& other) noexcept;
  void copy_construct(const DebugType& other);
  void copy_assign(const DebugType& other);
  void move_assign(DebugType&& other);

public:
  Tag tag;

  union {
    types::Null null;
    types::Variable variable;
    types::Scalar scalar;
    types::Union union_type;
    types::Abstraction abstraction;
    types::Tuple tuple;
    types::List list;
  };
};

const char* to_string(DebugType::Tag tag);
std::ostream& operator<<(std::ostream& stream, DebugType::Tag tag);

}

#undef MT_DEBUG_TYPE_RVALUE_CTOR
#undef MT_TYPE_VISITOR_METHOD