#include "unification.hpp"
#include "typing.hpp"
#include "library.hpp"
#include "debug.hpp"
#include "type_properties.hpp"
#include <algorithm>

#define MT_SHOW1(msg, a) \
  std::cout << (msg); \
  type_printer().show((a)); \
  std::cout << std::endl

#define MT_SHOW2(msg, a, b) \
  std::cout << (msg) << std::endl; \
  type_printer().show2((a), (b)); \
  std::cout << std::endl

namespace mt {

bool Unifier::is_concrete_argument(const mt::TypeHandle& handle) const {
  return TypeProperties(store).is_concrete_argument(handle);
}

bool Unifier::are_concrete_arguments(const mt::TypeHandles& handles) const {
  return TypeProperties(store).are_concrete_arguments(handles);
}

bool Unifier::is_known_subscript_type(const TypeHandle& handle) const {
  return is_concrete_argument(handle) && library.is_known_subscript_type(handle);
}

void Unifier::flatten_destructured_tuple(const types::DestructuredTuple& source, std::vector<TypeHandle>& into) const {
  for (const auto& mem : source.members) {
    if (type_of(mem) == Type::Tag::destructured_tuple) {
      flatten_destructured_tuple(store.at(mem).destructured_tuple, into);
    } else {
      into.push_back(mem);
    }
  }
}

void Unifier::check_push_func(const TypeHandle& source, const types::Abstraction& func) {
  if (registered_funcs.count(source) > 0) {
    return;
  }

  if (!is_concrete_argument(func.inputs)) {
    return;
  }

  assert(type_of(func.inputs) == Type::Tag::destructured_tuple);
  const auto& inputs = store.at(func.inputs).destructured_tuple;

  auto maybe_func = library.lookup_function(func);
  if (maybe_func) {
    const auto& func_ref = store.at(maybe_func.value());

    if (func_ref.is_abstraction()) {
      push_type_equation(TypeEquation(source, maybe_func.value()));

    } else {
      assert(func_ref.is_scheme());
      const auto& func_scheme = func_ref.scheme;
      const auto new_abstr_handle = instantiation.instantiate(func_scheme);
      assert(type_of(new_abstr_handle) == Type::Tag::abstraction);
      push_type_equation(TypeEquation(source, new_abstr_handle));
    }

  } else {
    std::cout << "ERROR: no such function: ";
    type_printer().show(source);
    std::cout << std::endl;
  }

  registered_funcs[source] = true;
}

void Unifier::check_assignment(const TypeHandle& source, const types::Assignment& assignment) {
  if (registered_assignments.count(source) > 0) {
    return;
  }

  if (is_concrete_argument(assignment.rhs)) {
    push_type_equation(TypeEquation(assignment.lhs, assignment.rhs));
    registered_assignments[source] = true;
  }
}

Optional<TypeHandle> Unifier::bound_type(const TypeHandle& for_type) const {
  if (bound_variables.count(for_type) == 0) {
    return NullOpt{};
  } else {
    return Optional<TypeHandle>(bound_variables.at(for_type));
  }
}

void Unifier::unify() {
  int64_t i = 0;
  while (i < type_equations.size()) {
    unify_one(type_equations[i++]);
  }

//  show();
}

TypeHandle Unifier::apply_to(const TypeHandle& source, mt::types::Variable& var) {
  if (bound_variables.count(source) > 0) {
    return bound_variables.at(source);
  } else {
    return source;
  }
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Tuple& tup) {
  apply_to(tup.members);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::DestructuredTuple& tup) {
  apply_to(tup.members);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Abstraction& func) {
  func.inputs = apply_to(func.inputs);
  func.outputs = apply_to(func.outputs);

  check_push_func(source, func);

  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Subscript& sub) {
  sub.principal_argument = apply_to(sub.principal_argument);
  for (auto& s : sub.subscripts) {
    apply_to(s.arguments);
  }
  sub.outputs = apply_to(sub.outputs);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::List& list) {
  apply_to(list.pattern);
  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source, types::Assignment& assignment) {
  assignment.lhs = apply_to(assignment.lhs);
  assignment.rhs = apply_to(assignment.rhs);

  check_assignment(source, assignment);

  return source;
}

TypeHandle Unifier::apply_to(const TypeHandle& source) {
  auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::variable:
      return apply_to(source, type.variable);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return apply_to(source, type.abstraction);
    case Type::Tag::tuple:
      return apply_to(source, type.tuple);
    case Type::Tag::destructured_tuple:
      return apply_to(source, type.destructured_tuple);
    case Type::Tag::subscript:
      return apply_to(source, type.subscript);
    case Type::Tag::list:
      return apply_to(source, type.list);
    case Type::Tag::assignment:
      return apply_to(source, type.assignment);
    default:
      MT_SHOW1("Unhandled apply to: ", source);
      assert(false);
      return source;
  }
}

void Unifier::apply_to(std::vector<TypeHandle>& sources) {
  for (auto& source : sources) {
    source = apply_to(source);
  }
}

TypeHandle Unifier::substitute_one(const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs) {
  auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::variable:
      return substitute_one(type.variable, source, lhs, rhs);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return substitute_one(type.abstraction, source, lhs, rhs);
    case Type::Tag::tuple:
      return substitute_one(type.tuple, source, lhs, rhs);
    case Type::Tag::destructured_tuple:
      return substitute_one(type.destructured_tuple, source, lhs, rhs);
    case Type::Tag::subscript:
      return substitute_one(type.subscript, source, lhs, rhs);
    case Type::Tag::list:
      return substitute_one(type.list, source, lhs, rhs);
    case Type::Tag::assignment:
      return substitute_one(type.assignment, source, lhs, rhs);
    default:
      MT_SHOW1("Unhandled: ", source);
      assert(false);
      return source;
  }
}

void Unifier::substitute_one(std::vector<TypeHandle>& sources, const TypeHandle& lhs, const TypeHandle& rhs) {
  for (auto& source : sources) {
    source = substitute_one(source, lhs, rhs);
  }
}

TypeHandle Unifier::substitute_one(types::Assignment& assignment, const TypeHandle& source,
                                     const TypeHandle& lhs, const TypeHandle& rhs) {
  assignment.rhs = substitute_one(assignment.rhs, lhs, rhs);
  assignment.lhs = substitute_one(assignment.lhs, lhs, rhs);

  check_assignment(source, assignment);

  return source;
}

TypeHandle Unifier::substitute_one(types::Abstraction& func, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  func.inputs = substitute_one(func.inputs, lhs, rhs);
  func.outputs = substitute_one(func.outputs, lhs, rhs);

  check_push_func(source, func);

  return source;
}

TypeHandle Unifier::substitute_one(types::Tuple& tup, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  substitute_one(tup.members, lhs, rhs);
  return source;
}

TypeHandle Unifier::substitute_one(types::DestructuredTuple& tup, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  substitute_one(tup.members, lhs, rhs);
  return source;
}

TypeHandle Unifier::substitute_one(types::Variable& var, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  if (source == lhs) {
    return rhs;
  } else {
    return source;
  }
}

TypeHandle Unifier::substitute_one(types::Subscript& sub, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  sub.principal_argument = substitute_one(sub.principal_argument, lhs, rhs);

  for (auto& s : sub.subscripts) {
    substitute_one(s.arguments, lhs, rhs);
  }

  sub.outputs = substitute_one(sub.outputs, lhs, rhs);
  return maybe_unify_subscript(source, sub);
}

void Unifier::flatten_list(const TypeHandle& source, std::vector<TypeHandle>& into) const {
  const auto& mem = store.at(source);

  switch (mem.tag) {
    case Type::Tag::list: {
      auto sz = mem.list.size();
      for (int64_t i = 0; i < sz; i++) {
        flatten_list(mem.list.pattern[i], into);
      }
      break;
    }
    case Type::Tag::destructured_tuple: {
      const auto& tup = mem.destructured_tuple;
      const auto sz = tup.is_outputs() ? std::min(int64_t(1), tup.size()) : tup.size();
      for (int64_t i = 0; i < sz; i++) {
        flatten_list(tup.members[i], into);
      }
      break;
    }
    default:
      into.push_back(source);
  }
}

TypeHandle Unifier::substitute_one(types::List& list, const TypeHandle& source,
                                   const TypeHandle& lhs, const TypeHandle& rhs) {
  std::vector<TypeHandle> flattened;
  flatten_list(source, flattened);
  std::swap(list.pattern, flattened);

  TypeHandle last;
  int64_t remove_from = 1;
  int64_t num_remove = 0;

  for (int64_t i = 0; i < list.size(); i++) {
    auto& element = list.pattern[i];
    element = substitute_one(element, lhs, rhs);
    assert(element.is_valid());

    const bool should_remove = i > 0 && is_concrete_argument(element) &&
      is_concrete_argument(last) && TypeEquality(store).equivalence_entry(element, last);

    if (should_remove) {
      num_remove++;
    } else {
      num_remove = 0;
      remove_from = i + 1;
    }
    last = element;
  }

  if (num_remove > 0) {
    list.pattern.erase(list.pattern.begin() + remove_from, list.pattern.end());
  }

  return source;
}

void Unifier::unify_one(TypeEquation eq) {
  using Tag = Type::Tag;

  eq.lhs = apply_to(eq.lhs);
  eq.rhs = apply_to(eq.rhs);

  if (eq.lhs == eq.rhs) {
    return;
  }

  const auto lhs_type = type_of(eq.lhs);
  const auto rhs_type = type_of(eq.rhs);

  if (lhs_type != Tag::variable && rhs_type != Tag::variable) {
    bool success = simplifier.simplify_entry(eq.lhs, eq.rhs);

    if (!success) {
      MT_SHOW2("Failed to simplify: ", eq.lhs, eq.rhs);
      assert(false);
    }

    return;

  } else if (lhs_type != Tag::variable) {
    assert(rhs_type == Tag::variable && "Rhs should be variable.");
    std::swap(eq.lhs, eq.rhs);
  }

  for (auto& subst_it : bound_variables) {
    subst_it.second = substitute_one(subst_it.second, eq.lhs, eq.rhs);
  }

  bound_variables[eq.lhs] = eq.rhs;
}

TypeHandle Unifier::maybe_unify_subscript(const TypeHandle& source, types::Subscript& sub) {
  if (is_known_subscript_type(sub.principal_argument)) {
    return maybe_unify_known_subscript_type(source, sub);

  } else if (type_of(sub.principal_argument) == Type::Tag::abstraction) {
    return maybe_unify_function_call_subscript(source, store.at(sub.principal_argument).abstraction, sub);

  } else if (type_of(sub.principal_argument) != Type::Tag::variable) {
    MT_SHOW1("Tried to unify subscript with principal arg: ", sub.principal_argument);
    assert(false);
  }

  return source;
}

TypeHandle Unifier::maybe_unify_function_call_subscript(const TypeHandle& source,
                                                        const types::Abstraction& source_func,
                                                        types::Subscript& sub) {
  if (registered_funcs.count(source) > 0) {
    return source;
  }

  if (sub.subscripts.size() != 1 || sub.subscripts[0].method != SubscriptMethod::parens) {
    registered_funcs[source] = true;
    std::cout << "ERROR: Expected exactly 1 () subscript." << std::endl;
    return source;
  }

  const auto& sub0 = sub.subscripts[0];

  if (!are_concrete_arguments(sub0.arguments)) {
    return source;
  }

  auto args_copy = sub0.arguments;
  const auto tup = store.make_rvalue_destructured_tuple(std::move(args_copy));
  const auto result_type = store.make_fresh_type_variable_reference();
  auto lookup = types::Abstraction::clone(source_func, tup, result_type);
  auto lookup_handle = store.make_type();
  store.assign(lookup_handle, Type(std::move(lookup)));

  push_type_equation(TypeEquation(sub.outputs, result_type));
  push_type_equation(TypeEquation(sub.principal_argument, lookup_handle));

  check_push_func(lookup_handle, store.at(lookup_handle).abstraction);
  registered_funcs[source] = true;

  return source;
}

TypeHandle Unifier::maybe_unify_known_subscript_type(const TypeHandle& source, types::Subscript& sub) {
  using types::Subscript;
  using types::Abstraction;
  using types::DestructuredTuple;

  if (registered_funcs.count(source) > 0) {
    return source;
  }

  assert(!sub.subscripts.empty() && "Expected at least one subscript.");
  const auto& sub0 = sub.subscripts[0];

  if (!are_concrete_arguments(sub0.arguments)) {
    return source;
  }

  std::vector<TypeHandle> into;
  auto args = sub0.arguments;
  args.insert(args.begin(), sub.principal_argument);

  DestructuredTuple tup(DestructuredTuple::Usage::rvalue, std::move(args));

  auto tup_type = store.make_type();
  store.assign(tup_type, Type(std::move(tup)));
  Abstraction abstraction(sub0.method, tup_type, TypeHandle());

  auto maybe_func = library.lookup_function(abstraction);
  if (maybe_func) {
//    MT_SHOW1("OK: found subscript signature: ", tup_type);
    const auto& func = store.at(maybe_func.value());

    if (func.is_abstraction()) {
      push_type_equation(TypeEquation(sub.outputs, func.abstraction.outputs));

    } else {
      assert(func.tag == Type::Tag::scheme);
      const auto& func_scheme = func.scheme;
      const auto new_abstr_handle = instantiation.instantiate(func_scheme);
      const auto& func_ref = store.at(new_abstr_handle);
      assert(func_ref.is_abstraction());

      push_type_equation(TypeEquation(sub.outputs, func_ref.abstraction.outputs));
      push_type_equation(TypeEquation(tup_type, func_ref.abstraction.inputs));
    }

    if (sub.subscripts.size() > 1) {
      auto result_type = store.make_fresh_type_variable_reference();
      push_type_equation(TypeEquation(result_type, sub.outputs));

      sub.principal_argument = result_type;
      sub.subscripts.erase(sub.subscripts.begin());
      sub.outputs = store.make_fresh_type_variable_reference();

    } else {
      registered_funcs[source] = true;
    }
  } else {
    MT_SHOW1("ERROR: no such subscript signature: ", tup_type);
    registered_funcs[source] = true;
  }

  return source;
}

Type::Tag Unifier::type_of(const mt::TypeHandle& handle) const {
  return store.at(handle).tag;
}

DebugTypePrinter Unifier::type_printer() const {
  return DebugTypePrinter(store, &string_registry);
}

void Unifier::show() {
  std::cout << "----" << std::endl;
  for (const auto& t : store.get_types()) {
    if (t.tag == Type::Tag::abstraction) {
      type_printer().show(t.abstraction);
      std::cout << std::endl;
    }
  }
}

}
