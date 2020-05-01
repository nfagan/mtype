#pragma once

#include "../utility.hpp"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace mt {

struct Token;
class TypeToString;
class IsFullyConcrete;
class TypeVisitor;

struct Type {
  struct Less {
    bool operator()(const Type* a, const Type* b) const noexcept;
  };
  struct Compare {
    int operator()(const Type* a, const Type* b) const noexcept;
  };
  struct Equal {
    bool operator()(const Type* a, const Type* b) const noexcept;
  };

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
    constant_value,
    scheme,
    assignment,
    parameters,
    class_type,
    record,
    alias,
    application,
    cast
  };

  Type() = delete;

  explicit Type(Tag tag) : tag(tag) {
    //
  }

  virtual ~Type() = default;

  virtual std::size_t bytes() const = 0;
  virtual void accept(const TypeToString& to_str, std::stringstream& into) const = 0;
  virtual bool accept(const IsFullyConcrete& is_fully_concrete) const = 0;
  virtual void accept(TypeVisitor& vis) = 0;
  virtual void accept_const(TypeVisitor& vis) const = 0;

  virtual Type* alias_source() {
    return this;
  }

  virtual const Type* alias_source() const {
    return this;
  }

  virtual const Type* scheme_source() const {
    return this;
  }

  virtual Type* scheme_source() {
    return this;
  }

  virtual int compare(const Type* other) const noexcept = 0;

  MT_NODISCARD bool is_scalar() const {
    return tag == Tag::scalar;
  }

  MT_NODISCARD bool is_tuple() const {
    return tag == Tag::tuple;
  }

  MT_NODISCARD bool is_variable() const {
    return tag == Tag::variable;
  }

  MT_NODISCARD bool is_destructured_tuple() const {
    return tag == Tag::destructured_tuple;
  }

  MT_NODISCARD bool is_list() const {
    return tag == Tag::list;
  }

  MT_NODISCARD bool is_abstraction() const {
    return tag == Tag::abstraction;
  }

  MT_NODISCARD bool is_application() const {
    return tag == Tag::application;
  }

  MT_NODISCARD bool is_scheme() const {
    return tag == Tag::scheme;
  }

  MT_NODISCARD bool is_parameters() const {
    return tag == Tag::parameters;
  }

  MT_NODISCARD bool is_class() const {
    return tag == Tag::class_type;
  }

  MT_NODISCARD bool is_record() const {
    return tag == Tag::record;
  }

  MT_NODISCARD bool is_constant_value() const {
    return tag == Tag::constant_value;
  }

  MT_NODISCARD bool is_alias() const {
    return tag == Tag::alias;
  }

  MT_NODISCARD bool is_union() const {
    return tag == Tag::union_type;
  }

  MT_NODISCARD bool is_cast() const {
    return tag == Tag::cast;
  }

  Tag tag;
};

using TypePtrs = std::vector<Type*>;

const char* to_string(Type::Tag tag);
std::ostream& operator<<(std::ostream& stream, Type::Tag tag);

}