#include "instance.hpp"
#include <cassert>

namespace mt {

void Instantiation::make_instance_variables(const types::Scheme& from_scheme, InstanceVariables& into) {
  for (const auto& param : from_scheme.parameters) {
    if (into.count(param) == 0) {
      const auto& p = store.at(param);
      const auto new_var = p.is_parameters() ? store.make_fresh_parameters() : store.make_fresh_type_variable_reference();
      into.emplace(param, new_var);
    }
  }
}

Instantiation::InstanceVariables Instantiation::make_instance_variables(const types::Scheme& from_scheme) {
  InstanceVariables replacing;
  make_instance_variables(from_scheme, replacing);
  return replacing;
}

TypeHandle Instantiation::instantiate(const types::Scheme& scheme) {
  InstanceVariables vars;
  make_instance_variables(scheme, vars);
  return instantiate(scheme, vars);
}

TypeHandle Instantiation::instantiate(const types::Scheme& scheme, IV replacing) {
  return clone(scheme.type, replacing);
}

TypeHandle Instantiation::clone(const TypeHandle& source, IV replacing) {
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
    case Type::Tag::subscript:
      return clone(type.subscript, replacing);
    case Type::Tag::scheme:
      return clone(type.scheme, replacing);
    case Type::Tag::parameters:
      return clone(type.parameters, source, replacing);
    default:
      std::cout << to_string(type.tag) << std::endl;
      assert(false && "Unhandled.");
      return source;
  }
}

TypeHandle Instantiation::clone(const types::DestructuredTuple& tup, IV replacing) {
  return store.make_destructured_tuple(tup.usage, clone(tup.members, replacing));
}

TypeHandle Instantiation::clone(const types::Abstraction& abstr, IV replacing) {
  const auto new_inputs = clone(abstr.inputs, replacing);
  const auto new_outputs = clone(abstr.outputs, replacing);
  auto new_abstr = types::Abstraction::clone(abstr, new_inputs, new_outputs);
  return store.make_abstraction(std::move(new_abstr));
}

TypeHandle Instantiation::clone(const types::Tuple& tup, IV replacing) {
  return store.make_tuple(clone(tup.members, replacing));
}

std::vector<TypeHandle> Instantiation::clone(const TypeHandles& a, IV replacing) {
  std::vector<TypeHandle> res;
  res.reserve(a.size());
  for (const auto& mem : a) {
    res.push_back(clone(mem, replacing));
  }
  return res;
}

TypeHandle Instantiation::clone(const types::List& list, IV replacing) {
  return store.make_list(clone(list.pattern, replacing));
}

TypeHandle Instantiation::clone(const types::Subscript& sub, IV replacing) {
  auto sub_b = sub;
  sub_b.principal_argument = clone(sub_b.principal_argument, replacing);
  sub_b.outputs = clone(sub_b.outputs, replacing);
  for (auto& s : sub_b.subscripts) {
    for (auto& arg : s.arguments) {
      arg = clone(arg, replacing);
    }
  }
  return store.make_subscript(std::move(sub_b));
}

TypeHandle Instantiation::clone(const types::Variable& var, TypeRef source, IV replacing) {
  if (replacing.count(source) > 0) {
    return replacing.at(source);
  } else {
    return source;
  }
}

TypeHandle Instantiation::clone(const types::Parameters& params, TypeRef source, IV replacing) {
  if (replacing.count(source) > 0) {
    return replacing.at(source);
  } else {
    return source;
  }
}

TypeHandle Instantiation::clone(const types::Scalar& scl, TypeRef source, IV replacing) {
  return source;
}

TypeHandle Instantiation::clone(const types::Scheme& scheme, IV replacing) {
  auto scheme_b = scheme;
  auto& new_replacing = replacing;
  make_instance_variables(scheme_b, new_replacing);

  scheme_b.parameters = clone(scheme_b.parameters, new_replacing);
  scheme_b.type = clone(scheme_b.type, new_replacing);

  for (auto& eq : scheme_b.constraints) {
    eq.lhs.term = clone(eq.lhs.term, new_replacing);
    eq.rhs.term = clone(eq.rhs.term, new_replacing);
  }
  return store.make_scheme(std::move(scheme_b));
}

}
