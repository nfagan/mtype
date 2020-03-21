#pragma once

#include "type.hpp"

namespace mt {

class TypeStore;

class TypeProperties {
public:
  explicit TypeProperties(const TypeStore& store) : store(store) {
    //
  }
public:
  bool is_concrete_argument(const TypeHandle& arg) const;
  bool are_concrete_arguments(const std::vector<TypeHandle>& args) const;

private:
  bool is_concrete_argument(const types::Abstraction& abstr) const;
  bool is_concrete_argument(const types::DestructuredTuple& tup) const;
  bool is_concrete_argument(const types::List& list) const;

private:
  const TypeStore& store;
};

}