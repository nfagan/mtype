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

struct Type {
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
    alias
  };

  Type() = delete;

  explicit Type(Tag tag) : tag(tag) {
    //
  }

  virtual ~Type() = default;

  virtual std::size_t bytes() const = 0;
  virtual void accept(const TypeToString& to_str, std::stringstream& into) const = 0;

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

  Tag tag;
};

using TypePtrs = std::vector<Type*>;

const char* to_string(Type::Tag tag);
std::ostream& operator<<(std::ostream& stream, Type::Tag tag);

}