#pragma once

#include "type.hpp"

namespace mt {

class TypeStore {
public:
  TypeStore() : type_variable_ids(0) {
    //
  }

  void reserve(int64_t cap) {
    types.reserve(cap);
  }

  const std::vector<Type>& get_types() const {
    return types;
  }

  [[nodiscard]] const Type& at(const TypeHandle& handle) const;
  Type& at(const TypeHandle& handle);

  TypeHandle make_type() {
    types.emplace_back();
    return TypeHandle(types.size() - 1);
  }

  TypeHandle make_fresh_type_variable_reference() {
    auto handle = make_type();
    assign(handle, Type(make_type_variable()));
    return handle;
  }

  types::Variable make_type_variable() {
    return types::Variable(make_type_identifier());
  }

  TypeIdentifier make_type_identifier() {
    return TypeIdentifier(type_variable_ids++);
  }

  template <typename T, typename... Args>
  TypeHandle make_type(Args&&... args) {
    types.emplace_back(Type(T(std::forward<Args>(args)...)));
    return TypeHandle(types.size()-1);
  }

  template <typename... Args>
  TypeHandle make_destructured_tuple(Args&&... args) {
    return make_type<types::DestructuredTuple>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeHandle make_list(Args&&... args) {
    return make_type<types::List>(std::forward<Args>(args)...);
  }

  TypeHandle make_concrete() {
    auto t = make_type();
    assign(t, Type(types::Scalar(make_type_identifier())));
    return t;
  }

  void make_builtin_types() {
    double_type_handle = make_concrete();
    string_type_handle = make_concrete();
    char_type_handle = make_concrete();
  }

  void assign(const TypeHandle& at, Type&& type);

private:
  std::vector<Type> types;
  int64_t type_variable_ids;

public:
  TypeHandle double_type_handle;
  TypeHandle string_type_handle;
  TypeHandle char_type_handle;
};

}