#include "instance.hpp"
#include <cassert>

namespace mt {

Instantiation::InstanceVariables Instantiation::make_instance_variables(const types::Scheme& from_scheme) {
  InstanceVariables replacing;

  for (const auto& param : from_scheme.parameters) {
    if (replacing.count(param) == 0) {
      replacing.emplace(param, store.make_fresh_type_variable_reference());
    }
  }

  return replacing;
}

TypeHandle Instantiation::instantiate(const types::Scheme& scheme) {
  ClonedVariables cloned;
  BoundTerms preserving;
  return instantiate(scheme, preserving, cloned);
}

TypeHandle Instantiation::instantiate(const types::Scheme& scheme, BT preserving, CV cloned) {
  return instantiate(scheme, make_instance_variables(scheme), preserving, cloned);
}

TypeHandle Instantiation::instantiate(const types::Scheme& scheme, IV replacing, BT preserving, CV cloned) {
  return clone(scheme.type, replacing, preserving, cloned);
}

TypeHandle Instantiation::clone(const TypeHandle& source, IV replacing, BT preserving, CV cloned) {
  const auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::abstraction:
      return clone(type.abstraction, replacing, preserving, cloned);
    case Type::Tag::destructured_tuple:
      return clone(type.destructured_tuple, replacing, preserving, cloned);
    case Type::Tag::tuple:
      return clone(type.tuple, replacing, preserving, cloned);
    case Type::Tag::list:
      return clone(type.list, replacing, preserving, cloned);
    case Type::Tag::variable:
      return clone(type.variable, source, replacing, preserving, cloned);
    case Type::Tag::scalar:
      return clone(type.scalar, source, replacing, preserving, cloned);
    case Type::Tag::subscript:
      return clone(type.subscript, replacing, preserving, cloned);
    case Type::Tag::scheme:
      return clone(type.scheme, replacing, preserving, cloned);
    default:
      std::cout << to_string(type.tag) << std::endl;
      assert(false && "Unhandled.");
      return source;
  }
}

TypeHandle Instantiation::clone(const types::DestructuredTuple& tup, IV replacing, BT preserving, CV cloned) {
  return store.make_destructured_tuple(tup.usage, clone(tup.members, replacing, preserving, cloned));
}

TypeHandle Instantiation::clone(const types::Abstraction& abstr, IV replacing, BT preserving, CV cloned) {
  const auto new_inputs = clone(abstr.inputs, replacing, preserving, cloned);
  const auto new_outputs = clone(abstr.outputs, replacing, preserving, cloned);
  auto new_abstr = types::Abstraction::clone(abstr, new_inputs, new_outputs);
  const auto new_type = store.make_type();
  store.assign(new_type, Type(std::move(new_abstr)));
  return new_type;
}

TypeHandle Instantiation::clone(const types::Tuple& tup, IV replacing, BT preserving, CV cloned) {
  auto new_tup_handle = store.make_type();
  types::Tuple new_tup(clone(tup.members, replacing, preserving, cloned));
  store.assign(new_tup_handle, Type(std::move(new_tup)));
  return new_tup_handle;
}

std::vector<TypeHandle> Instantiation::clone(const TypeHandles& a, IV replacing, BT preserving, CV cloned) {
  std::vector<TypeHandle> res;
  res.reserve(a.size());
  for (const auto& mem : a) {
    res.push_back(clone(mem, replacing, preserving, cloned));
  }
  return res;
}

TypeHandle Instantiation::clone(const types::List& list, IV replacing, BT preserving, CV cloned) {
  return store.make_list(clone(list.pattern, replacing, preserving, cloned));
}

TypeHandle Instantiation::clone(const types::Subscript& sub, IV replacing, BT preserving, CV cloned) {
  auto sub_b = sub;
  sub_b.principal_argument = clone(sub_b.principal_argument, replacing, preserving, cloned);
  sub_b.outputs = clone(sub_b.outputs, replacing, preserving, cloned);
  for (auto& s : sub_b.subscripts) {
    for (auto& arg : s.arguments) {
      arg = clone(arg, replacing, preserving, cloned);
    }
  }
  auto new_sub = store.make_type();
  store.assign(new_sub, Type(std::move(sub_b)));
  return new_sub;
}

TypeHandle Instantiation::clone(const types::Variable& var, TypeRef source, IV replacing, BT preserving, CV cloned) {
  if (replacing.count(source) > 0) {
    return replacing.at(source);

  } else if (preserving.count(make_term(nullptr, source)) > 0) {
    assert(false);
    return source;

  } else if (cloned.count(source) > 0) {
    return cloned.at(source);

  } else {
    const auto new_source = store.make_fresh_type_variable_reference();
    cloned[source] = new_source;
    return new_source;
  }
}

TypeHandle Instantiation::clone(const types::Scalar& scl, TypeRef source, IV replacing, BT preserving, CV cloned) {
  return source;
}

TypeHandle Instantiation::clone(const types::Scheme& scheme, IV replacing, BT preserving, CV cloned) {
  auto scheme_b = scheme;
  scheme_b.parameters = clone(scheme_b.parameters, replacing, preserving, cloned);
  scheme_b.type = clone(scheme_b.type, replacing, preserving, cloned);
  for (auto& eq : scheme_b.constraints) {
    eq.lhs.term = clone(eq.lhs.term, replacing, preserving, cloned);
    eq.rhs.term = clone(eq.rhs.term, replacing, preserving, cloned);
  }
  auto scheme_handle = store.make_type();
  store.assign(scheme_handle, Type(std::move(scheme_b)));
  return scheme_handle;
}

}
