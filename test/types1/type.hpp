#pragma once

#include "mt/mt.hpp"
#include <cstdint>
#include <iostream>

#define MT_TYPE_VISITOR_METHOD(name, type) \
  virtual void name(type&) {} \
  virtual void name(const type&) {}

namespace mt {

class TypeVisitor;
class TypeExprVisitor;

struct TypeHandle : public detail::Handle<100> {
  friend class TypeVisitor;
  using Handle::Handle;
};

struct TypeExprHandle : public detail::Handle<101> {
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

namespace texpr {
  struct Expr {
    Expr() = default;
    virtual ~Expr() = default;

    virtual bool is_terminal(const TypeVisitor& visitor) const {
      return false;
    }
    virtual void accept_const(TypeExprVisitor&) const {
      //
    }
    virtual void accept(TypeExprVisitor&) {
      //
    }
  };

  using BoxedExpr = std::unique_ptr<texpr::Expr>;

  struct TypeReference : public Expr {
    explicit TypeReference(const TypeHandle& handle) : type_handle(handle) {
      //
    }

    bool is_terminal(const TypeVisitor& visitor) const override {
      return true;
    }

    void accept(TypeExprVisitor& visitor) override;
    void accept_const(TypeExprVisitor& visitor) const override;

    TypeHandle type_handle;
  };

  struct Application : public Expr {
    enum class Type : uint8_t {
      unary_operator = 0,
      binary_operator,
      function,
    };

    struct Less {
      bool operator()(const Application& a, const Application& b) const;
    };

    Application(BinaryOperator binary_operator, std::vector<TypeHandle>&& args, const TypeHandle& result) :
    type(Type::binary_operator),
    binary_operator(binary_operator),
    arguments(std::move(args)),
    outputs{result} {
      //
    }
    Application(UnaryOperator unary_operator, std::vector<TypeHandle>&& args, const TypeHandle& result) :
      type(Type::unary_operator),
      unary_operator(unary_operator),
      arguments(std::move(args)),
      outputs{result} {
      //
    }
    Application(std::vector<TypeHandle>&& args, std::vector<TypeHandle>&& outputs) :
      type(Type::function), arguments(std::move(args)), outputs(std::move(outputs)) {
      //
    }
    ~Application() override = default;

    void accept(TypeExprVisitor& visitor) override;
    void accept_const(TypeExprVisitor& visitor) const override;

    bool operator==(const Application& other) const;

    Type type;

    union {
      BinaryOperator binary_operator;
      UnaryOperator unary_operator;
    };

    std::vector<TypeHandle> arguments;
    std::vector<TypeHandle> outputs;
  };
}

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

    std::vector<TypeHandle> types;
  };

  struct Function {
    Function() = default;
    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(Function)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(Function)

    std::vector<TypeHandle> outputs;
    std::vector<TypeHandle> inputs;
  };
}

class DebugType {
public:
  enum class Tag {
    null,
    variable,
    scalar,
    function,
    union_type
  };

  DebugType() : DebugType(types::Null{}) {
    //
  }

  explicit DebugType(types::Null&& null) : tag(Tag::null) {
    //
  }
  explicit DebugType(types::Variable&& var) : tag(Tag::variable) {
    new (&variable) types::Variable(std::move(var));
  }
  explicit DebugType(types::Scalar&& scl) : tag(Tag::scalar) {
    new (&scalar) types::Scalar(std::move(scl));
  }
  explicit DebugType(types::Function&& func) : tag(Tag::function) {
    new (&function) types::Function(std::move(func));
  }
  explicit DebugType(types::Union&& un) : tag(Tag::union_type) {
    new (&union_type) types::Union(std::move(un));
  }

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
    types::Function function;
  };
};

const char* to_string(DebugType::Tag tag);
std::ostream& operator<<(std::ostream& stream, DebugType::Tag tag);

class TypeExprVisitor {
public:
  MT_TYPE_VISITOR_METHOD(type_reference, texpr::TypeReference)
  MT_TYPE_VISITOR_METHOD(application, texpr::Application)
};

}

#undef MT_TYPE_VISITOR_METHOD