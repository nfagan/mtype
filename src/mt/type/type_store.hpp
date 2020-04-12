#pragma once

#include "types.hpp"
#include <utility>
#include <memory>

namespace mt {

class TypeStore {
public:
  TypeStore() = delete;

  explicit TypeStore(int64_t cap) : capacity(cap), type_variable_ids(0) {
    reserve();
  }

  TypeStore(const TypeStore& other) = delete;
  TypeStore& operator=(const TypeStore& other) = delete;

  int64_t size() const {
    return types.size();
  }

  Type* make_fresh_parameters() {
    return make_type<types::Parameters>(make_type_identifier());
  }

  Type* make_fresh_type_variable_reference() {
    return make_type<types::Variable>(make_type_identifier());
  }

  template <typename... Args>
  Type* make_constant_value(Args&&... args) {
    return make_type<types::ConstantValue>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_tuple(Args&&... args) {
    return make_type<types::Tuple>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_destructured_tuple(Args&&... args) {
    return make_type<types::DestructuredTuple>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_output_destructured_tuple(Args&&... args) {
    using Use = types::DestructuredTuple::Usage;
    return make_type<types::DestructuredTuple>(Use::definition_outputs, std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_input_destructured_tuple(Args&&... args) {
    using Use = types::DestructuredTuple::Usage;
    return make_type<types::DestructuredTuple>(Use::definition_inputs, std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_rvalue_destructured_tuple(Args&&... args) {
    using Use = types::DestructuredTuple::Usage;
    return make_type<types::DestructuredTuple>(Use::rvalue, std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_lvalue_destructured_tuple(Args&&... args) {
    using Use = types::DestructuredTuple::Usage;
    return make_type<types::DestructuredTuple>(Use::lvalue, std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_list(Args&&... args) {
    return make_type<types::List>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_assignment(Args&&... args) {
    return make_type<types::Assignment>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_abstraction(Args&&... args) {
    return make_type<types::Abstraction>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_scheme(Args&&... args) {
    return make_type<types::Scheme>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  Type* make_subscript(Args&&... args) {
    return make_type<types::Subscript>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  types::Class* make_class(Args&&... args) {
    return make_type<types::Class>(std::forward<Args>(args)...);
  }

  Type* make_scalar(const TypeIdentifier& id) {
    return make_type<types::Scalar>(id);
  }

  template <typename... Args>
  types::Record* make_record(Args&&... args) {
    return make_type<types::Record>(std::forward<Args>(args)...);
  }

  template <typename... Args>
  TypeReference* make_type_reference(Args&&... args) {
    auto ref = std::make_unique<TypeReference>(std::forward<Args>(args)...);
    auto ptr = ref.get();
    type_refs.push_back(std::move(ref));
    return ptr;
  }

  std::unordered_map<Type::Tag, double> type_distribution() const;

private:
  void reserve() {
    types.reserve(capacity);
  }

  TypeIdentifier make_type_identifier() {
    return TypeIdentifier(type_variable_ids++);
  }

  template <typename T, typename... Args>
  T* make_type(Args&&... args) {
    auto type = std::make_unique<T>(std::forward<Args>(args)...);
    auto ptr = type.get();
    types.push_back(std::move(type));
    return ptr;
  }

private:
  std::vector<std::unique_ptr<Type>> types;
  std::vector<std::unique_ptr<TypeReference>> type_refs;
  std::size_t capacity;
  int64_t type_variable_ids;
};

}