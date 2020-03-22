#include "instance.hpp"
#include <cassert>

namespace mt {

Instantiation::InstanceVariables Instantiation::make_instance_variable(const types::Scheme& from_scheme) {
  InstanceVariables replacing;

  for (const auto& param : from_scheme.parameters) {
    if (replacing.count(param) == 0) {
      replacing.emplace(param, store.make_fresh_type_variable_reference());
    }
  }

  return replacing;
}

TypeHandle Instantiation::instantiate(const types::Scheme& scheme) {
  return clone(scheme.type, make_instance_variable(scheme));
}

TypeHandle Instantiation::clone(const TypeHandle& source, const InstanceVariables& replacing) {
  const auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::abstraction:
      return clone(type.abstraction, replacing);
    case Type::Tag::destructured_tuple:
      return clone(type.destructured_tuple, replacing);
    case Type::Tag::tuple:
      return clone(type.tuple, replacing);
    case Type::Tag::list:
      return clone(type.list, replacing);
    case Type::Tag::variable:
      return clone(type.variable, source, replacing);
    case Type::Tag::scalar:
      return clone(type.scalar, source, replacing);
    default:
      std::cout << to_string(type.tag) << std::endl;
      assert(false && "Unhandled.");
      return source;
  }
}

TypeHandle Instantiation::clone(const types::DestructuredTuple& tup, const InstanceVariables& replacing) {
  return store.make_destructured_tuple(tup.usage, clone(tup.members, replacing));
}

TypeHandle Instantiation::clone(const types::Abstraction& abstr, const InstanceVariables& replacing) {
  const auto new_inputs = clone(abstr.inputs, replacing);
  const auto new_outputs = clone(abstr.outputs, replacing);
  auto new_abstr = types::Abstraction::clone(abstr, new_inputs, new_outputs);
  const auto new_type = store.make_type();
  store.assign(new_type, Type(std::move(new_abstr)));
  return new_type;
}

TypeHandle Instantiation::clone(const types::Tuple& tup, const InstanceVariables& replacing) {
  auto new_tup_handle = store.make_type();
  types::Tuple new_tup(clone(tup.members, replacing));
  store.assign(new_tup_handle, Type(std::move(new_tup)));
  return new_tup_handle;
}

std::vector<TypeHandle> Instantiation::clone(const std::vector<TypeHandle>& a, const InstanceVariables& replacing) {
  std::vector<TypeHandle> res;
  res.reserve(a.size());
  for (const auto& mem : a) {
    res.push_back(clone(mem, replacing));
  }
  return res;
}

TypeHandle Instantiation::clone(const types::List& list, const InstanceVariables& replacing) {
  return store.make_list(clone(list.pattern, replacing));
}

TypeHandle Instantiation::clone(const types::Variable& var, const TypeHandle& source, const InstanceVariables& replacing) {
  if (replacing.count(source) > 0) {
    return replacing.at(source);
  } else {
    return source;
  }
}

TypeHandle Instantiation::clone(const types::Scalar& scl, const TypeHandle& source, const InstanceVariables& replacing) {
  return source;
}

}
