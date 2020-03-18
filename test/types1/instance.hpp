#pragma once

#include "type_store.hpp"
#include "type.hpp"

namespace mt {

class Instantiation {
  using InstanceVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;

public:
  explicit Instantiation(TypeStore& store) : store(store) {
    //
  }

  TypeHandle instantiate(const types::Scheme& scheme);

  TypeHandle clone(const TypeHandle& handle, const InstanceVariables& replacing);
  TypeHandle clone(const types::Abstraction& abstr, const InstanceVariables& replacing);
  TypeHandle clone(const types::DestructuredTuple& tup, const InstanceVariables& replacing);
  TypeHandle clone(const types::Tuple& tup, const InstanceVariables& replacing);
  TypeHandle clone(const types::List& list, const InstanceVariables& replacing);
  TypeHandle clone(const types::Variable& var, const TypeHandle& source, const InstanceVariables& replacing);
  TypeHandle clone(const types::Scalar& scl, const TypeHandle& source, const InstanceVariables& replacing);

private:
  std::vector<TypeHandle> clone(const std::vector<TypeHandle>& members, const InstanceVariables& replacing);
  InstanceVariables make_instance_variable(const types::Scheme& from_scheme);

private:
  TypeStore& store;
};

}