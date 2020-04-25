#pragma once

#include "type_store.hpp"
#include "types.hpp"
#include <unordered_map>

namespace mt {

class Instantiation {
public:
  using InstanceVars = std::unordered_map<Type*, Type*>;

public:
  explicit Instantiation(TypeStore& store) : store(store) {
    //
  }

  Type* instantiate(const types::Scheme& scheme, InstanceVars& replacing);

  Type* clone(Type* source, InstanceVars& replacing);
  InstanceVars make_instance_variables(const types::Scheme& from_scheme);

private:
  void make_instance_variables(const types::Scheme& from_scheme, InstanceVars& into);

  Type* clone(const types::Abstraction& abstr, InstanceVars& replacing);
  Type* clone(const types::Application& app, InstanceVars& replacing);
  Type* clone(const types::DestructuredTuple& tup, InstanceVars& replacing);
  Type* clone(const types::Tuple& tup, InstanceVars& replacing);
  Type* clone(const types::Union& union_type, InstanceVars& replacing);
  Type* clone(const types::List& list, InstanceVars& replacing);
  Type* clone(const types::Subscript& sub, InstanceVars& replacing);
  Type* clone(const types::Scheme& scheme, InstanceVars& replacing);
  Type* clone(const types::Assignment& assign, InstanceVars& replacing);
  Type* clone(const types::Class& cls, InstanceVars& replacing);
  Type* clone(const types::Record& record, InstanceVars& replacing);
  Type* clone(const types::Alias& alias, InstanceVars& replacing);
  Type* clone(const types::Variable& var, Type* source, InstanceVars& replacing);
  Type* clone(const types::Scalar& scl, Type* source, InstanceVars& replacing);
  Type* clone(const types::Parameters& params, Type* source, InstanceVars& replacing);
  Type* clone(const types::ConstantValue& cv, Type* source, InstanceVars& replacing);

  TypePtrs clone(const TypePtrs& members, InstanceVars& replacing);

private:
  TypeStore& store;
};

}