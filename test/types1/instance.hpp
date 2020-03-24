#pragma once

#include "type_store.hpp"
#include "type.hpp"

namespace mt {

class Instantiation {
public:
  using InstanceVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;
  using ClonedVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;
  using IV = const InstanceVariables&;
  using BT = const BoundTerms&;
  using CV = ClonedVariables&;

public:
  explicit Instantiation(TypeStore& store) : store(store) {
    //
  }

  TypeHandle instantiate(const types::Scheme& scheme);
  TypeHandle instantiate(const types::Scheme& scheme, BT preserving, CV cloned);
  TypeHandle instantiate(const types::Scheme& scheme, IV replacing, BT preserving, CV cloned);

  TypeHandle clone(const TypeHandle& handle, IV replacing, BT preserving, CV cloned);
  InstanceVariables make_instance_variables(const types::Scheme& from_scheme);

private:
  TypeHandle clone(const types::Abstraction& abstr, IV replacing, BT preserving, CV cloned);
  TypeHandle clone(const types::DestructuredTuple& tup, IV replacing, BT preserving, CV cloned);
  TypeHandle clone(const types::Tuple& tup, IV replacing, BT preserving, CV cloned);
  TypeHandle clone(const types::List& list, IV replacing, BT preserving, CV cloned);
  TypeHandle clone(const types::Subscript& sub, IV replacing, BT preserving, CV cloned);
  TypeHandle clone(const types::Scheme& scheme, IV replacing, BT preserving, CV cloned);
  TypeHandle clone(const types::Variable& var, TypeRef source, IV replacing, BT preserving, CV cloned);
  TypeHandle clone(const types::Scalar& scl, TypeRef source, IV replacing, BT preserving, CV cloned);

  std::vector<TypeHandle> clone(const std::vector<TypeHandle>& members, IV replacing, BT preserving, CV cloned);

private:
  TypeStore& store;
};

}