#pragma once

#include "type_store.hpp"
#include "type.hpp"

namespace mt {

class Instantiation {
public:
  using InstanceVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;
  using IV = InstanceVariables&;

public:
  explicit Instantiation(TypeStore& store) : store(store) {
    //
  }

  TypeHandle instantiate(const types::Scheme& scheme);
  TypeHandle instantiate(const types::Scheme& scheme, IV replacing);

  TypeHandle clone(const TypeHandle& handle, IV replacing);
  InstanceVariables make_instance_variables(const types::Scheme& from_scheme);

private:
  void make_instance_variables(const types::Scheme& from_scheme, InstanceVariables& into);

  TypeHandle clone(const types::Abstraction& abstr, IV replacing);
  TypeHandle clone(const types::DestructuredTuple& tup, IV replacing);
  TypeHandle clone(const types::Tuple& tup, IV replacing);
  TypeHandle clone(const types::List& list, IV replacing);
  TypeHandle clone(const types::Subscript& sub, IV replacing);
  TypeHandle clone(const types::Scheme& scheme, IV replacing);
  TypeHandle clone(const types::Assignment& assign, IV replacing);
  TypeHandle clone(const types::Variable& var, TypeRef source, IV replacing);
  TypeHandle clone(const types::Scalar& scl, TypeRef source, IV replacing);
  TypeHandle clone(const types::Parameters& params, TypeRef source, IV replacing);

  std::vector<TypeHandle> clone(const std::vector<TypeHandle>& members, IV replacing);

private:
  TypeStore& store;
};

}