#pragma once

#include "type.hpp"

namespace mt {

class TypeStore {
public:
  TypeStore() : type_variable_ids(0) {
    //
  }

  explicit TypeStore(int64_t cap) : type_variable_ids(0) {
    reserve(cap);
  }

  TypeStore(const TypeStore& other) = delete;
  TypeStore& operator=(const TypeStore& other) = delete;

  void reserve(int64_t cap) {
    types.reserve(cap);
  }

  int64_t size() const {
    return types.size();
  }

  [[nodiscard]] const std::vector<Type>& get_types() const {
    return types;
  }

  [[nodiscard]] const Type& at(const TypeHandle& handle) const;
  Type& at(const TypeHandle& handle);

  Type::Tag type_of(const TypeHandle& handle) const {
    return at(handle).tag;
  }

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
  TypeHandle make_output_destructured_tuple(Args&&... args) {
    using Use = types::DestructuredTuple::Usage;
    return make_type<types::DestructuredTuple>(Use::definition_outputs, std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeHandle make_input_destructured_tuple(Args&&... args) {
    using Use = types::DestructuredTuple::Usage;
    return make_type<types::DestructuredTuple>(Use::definition_inputs, std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeHandle make_rvalue_destructured_tuple(Args&&... args) {
    using Use = types::DestructuredTuple::Usage;
    return make_type<types::DestructuredTuple>(Use::rvalue, std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeHandle make_list(Args&&... args) {
    return make_type<types::List>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeHandle make_assignment(Args&&... args) {
    return make_type<types::Assignment>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeHandle make_abstraction(Args&&... args) {
    return make_type<types::Abstraction>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeHandle make_scheme(Args&&... args) {
    return make_type<types::Scheme>(std::forward<Args>(args)...);
  }

  TypeHandle make_concrete() {
    auto t = make_type();
    assign(t, Type(types::Scalar(make_type_identifier())));
    return t;
  }

  void assign(const TypeHandle& at, Type&& type);

  std::unordered_map<Type::Tag, double> type_distribution() const {
    std::unordered_map<Type::Tag, double> counts;

    for (const auto& type : types) {
      if (counts.count(type.tag) == 0) {
        counts[type.tag] = 0.0;
      }
      counts[type.tag] = counts[type.tag] + 1.0;
    }

    return counts;
  }

private:
  std::vector<Type> types;
  int64_t type_variable_ids;
};

}