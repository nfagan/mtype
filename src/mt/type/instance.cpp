#include "instance.hpp"
#include <cassert>

namespace mt {

void Instantiation::make_instance_variables(const types::Scheme& from_scheme, InstanceVariables& into) {
  for (const auto& param : from_scheme.parameters) {
    if (into.count(param) == 0) {
      const auto new_var = param->is_parameters() ? store.make_fresh_parameters() : store.make_fresh_type_variable_reference();
      into.emplace(param, new_var);
    }
  }
}

Instantiation::InstanceVariables Instantiation::make_instance_variables(const types::Scheme& from_scheme) {
  InstanceVariables replacing;
  make_instance_variables(from_scheme, replacing);
  return replacing;
}

Type* Instantiation::instantiate(const types::Scheme& scheme) {
  InstanceVariables vars;
  make_instance_variables(scheme, vars);
  return instantiate(scheme, vars);
}

Type* Instantiation::instantiate(const types::Scheme& scheme, IV replacing) {
  return clone(scheme.type, replacing);
}

Type* Instantiation::clone(Type* source, IV replacing) {
  switch (source->tag) {
    case Type::Tag::abstraction:
      return clone(MT_ABSTR_REF(*source), replacing);
    case Type::Tag::destructured_tuple:
      return clone(MT_DT_REF(*source), replacing);
    case Type::Tag::tuple:
      return clone(MT_TUPLE_REF(*source), replacing);
    case Type::Tag::list:
      return clone(MT_LIST_REF(*source), replacing);
    case Type::Tag::variable:
      return clone(MT_VAR_REF(*source), source, replacing);
    case Type::Tag::scalar:
      return clone(MT_SCALAR_REF(*source), source, replacing);
    case Type::Tag::subscript:
      return clone(MT_SUBS_REF(*source), replacing);
    case Type::Tag::scheme:
      return clone(MT_SCHEME_REF(*source), replacing);
    case Type::Tag::assignment:
      return clone(MT_ASSIGN_REF(*source), replacing);
    case Type::Tag::parameters:
      return clone(MT_PARAMS_REF(*source), source, replacing);
    default:
      std::cout << to_string(source->tag) << std::endl;
      assert(false && "Unhandled.");
      return nullptr;
  }
}

Type* Instantiation::clone(const types::DestructuredTuple& tup, IV replacing) {
  return store.make_destructured_tuple(tup.usage, clone(tup.members, replacing));
}

Type* Instantiation::clone(const types::Abstraction& abstr, IV replacing) {
  auto new_abstr = abstr;
  new_abstr.inputs = clone(new_abstr.inputs, replacing);
  new_abstr.outputs = clone(new_abstr.outputs, replacing);
  return store.make_abstraction(std::move(new_abstr));
}

Type* Instantiation::clone(const types::Tuple& tup, IV replacing) {
  return store.make_tuple(clone(tup.members, replacing));
}

TypePtrs Instantiation::clone(const TypePtrs& a, IV replacing) {
  TypePtrs res;
  res.reserve(a.size());
  for (const auto& mem : a) {
    res.push_back(clone(mem, replacing));
  }
  return res;
}

Type* Instantiation::clone(const types::List& list, IV replacing) {
  return store.make_list(clone(list.pattern, replacing));
}

Type* Instantiation::clone(const types::Subscript& sub, IV replacing) {
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

Type* Instantiation::clone(const types::Variable&, Type* source, IV replacing) {
  if (replacing.count(source) > 0) {
    return replacing.at(source);
  } else {
    return source;
  }
}

Type* Instantiation::clone(const types::Parameters&, Type* source, IV replacing) {
  if (replacing.count(source) > 0) {
    return replacing.at(source);
  } else {
    return source;
  }
}

Type* Instantiation::clone(const types::Scalar&, Type* source, IV) {
  return source;
}

Type* Instantiation::clone(const types::Scheme& scheme, IV replacing) {
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

Type* Instantiation::clone(const types::Assignment& assign, IV replacing) {
  const auto lhs = clone(assign.lhs, replacing);
  const auto rhs = clone(assign.rhs, replacing);
  return store.make_assignment(lhs, rhs);
}

}
