#pragma once

#include "type_store.hpp"
#include "types.hpp"
#include <unordered_map>

namespace mt {

class Instantiation {
public:
  using InstanceVariables = std::unordered_map<Type*, Type*>;
  using IV = InstanceVariables&;

public:
  explicit Instantiation(TypeStore& store) : store(store) {
    //
  }

  Type* instantiate(const types::Scheme& scheme);
  Type* instantiate(const types::Scheme& scheme, IV replacing);

  Type* clone(Type* source, IV replacing);
  InstanceVariables make_instance_variables(const types::Scheme& from_scheme);

private:
  void make_instance_variables(const types::Scheme& from_scheme, InstanceVariables& into);

  Type* clone(const types::Abstraction& abstr, IV replacing);
  Type* clone(const types::DestructuredTuple& tup, IV replacing);
  Type* clone(const types::Tuple& tup, IV replacing);
  Type* clone(const types::List& list, IV replacing);
  Type* clone(const types::Subscript& sub, IV replacing);
  Type* clone(const types::Scheme& scheme, IV replacing);
  Type* clone(const types::Assignment& assign, IV replacing);
  Type* clone(const types::Variable& var, Type* source, IV replacing);
  Type* clone(const types::Scalar& scl, Type* source, IV replacing);
  Type* clone(const types::Parameters& params, Type* source, IV replacing);

  TypePtrs clone(const TypePtrs& members, IV replacing);

private:
  TypeStore& store;
};

}