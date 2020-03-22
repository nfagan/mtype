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

bool Unifier::is_known_subscript_type(TypeRef handle) const {
  return is_concrete_argument(handle) && library.is_known_subscript_type(handle);
}

void Unifier::check_push_func(TypeRef source, TermRef term, const types::Abstraction& func) {
  if (registered_funcs.count(source) > 0) {
    return;
  }

  if (!is_concrete_argument(func.inputs) || func.is_anonymous()) {
    return;
  }

  assert(type_of(func.inputs) == Type::Tag::destructured_tuple);
  const auto& inputs = store.at(func.inputs).destructured_tuple;

  auto maybe_func = library.lookup_function(func);
  if (maybe_func) {
    const auto& func_ref = store.at(maybe_func.value());

    if (func_ref.is_abstraction()) {
      const auto lhs_term = make_term(term.source_token, source);
      const auto rhs_term = make_term(term.source_token, maybe_func.value());
      substitution->push_type_equation(make_eq(lhs_term, rhs_term));

    } else {
      assert(func_ref.is_scheme());
      const auto& func_scheme = func_ref.scheme;
      const auto new_abstr_handle = instantiation.instantiate(func_scheme);
      assert(type_of(new_abstr_handle) == Type::Tag::abstraction);

      const auto lhs_term = make_term(term.source_token, source);
      const auto rhs_term = make_term(term.source_token, new_abstr_handle);
      substitution->push_type_equation(make_eq(lhs_term, rhs_term));
    }

  } else {
    std::cout << "ERROR: no such function: ";
    type_printer().show(source);
    std::cout << std::endl;
  }

  registered_funcs[source] = true;
}

void Unifier::check_assignment(TypeRef source, TermRef term, const types::Assignment& assignment) {
  if (registered_assignments.count(source) > 0) {
    return;
  }

  if (is_concrete_argument(assignment.rhs)) {
    const auto lhs_term = make_term(term.source_token, assignment.lhs);
    const auto rhs_term = make_term(term.source_token, assignment.rhs);
    substitution->push_type_equation(make_eq(lhs_term, rhs_term));

    registered_assignments[source] = true;
  }
}

void Unifier::reset(Substitution* subst) {
  assert(subst);

  substitution = subst;
  simplification_failures.clear();
  any_failures = false;
}

UnifyResult Unifier::unify(Substitution* subst) {
  reset(subst);

  int64_t i = 0;
  while (i < substitution->type_equations.size()) {
    unify_one(substitution->type_equations[i++]);
  }

  if (had_error()) {
    return UnifyResult(std::move(simplification_failures));
  } else {
    return UnifyResult();
  }
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term, types::Variable& var) {
  TypeEquationTerm lookup(nullptr, source);

  if (substitution->bound_variables.count(lookup) > 0) {
    return substitution->bound_variables.at(lookup).term;
  } else {
    return source;
  }
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term, types::Tuple& tup) {
  apply_to(tup.members, term);
  return source;
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term, types::DestructuredTuple& tup) {
  apply_to(tup.members, term);
  return source;
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term, types::Abstraction& func) {
  func.inputs = apply_to(func.inputs, term);
  func.outputs = apply_to(func.outputs, term);

  check_push_func(source, term, func);

  return source;
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term, types::Subscript& sub) {
  sub.principal_argument = apply_to(sub.principal_argument, term);
  for (auto& s : sub.subscripts) {
    apply_to(s.arguments, term);
  }
  sub.outputs = apply_to(sub.outputs, term);
  return source;
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term, types::List& list) {
  apply_to(list.pattern, term);
  return source;
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term, types::Assignment& assignment) {
  assignment.lhs = apply_to(assignment.lhs, term);
  assignment.rhs = apply_to(assignment.rhs, term);

  check_assignment(source, term, assignment);

  return source;
}

TypeHandle Unifier::apply_to(TypeRef source, TermRef term) {
  auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::variable:
      return apply_to(source, term, type.variable);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return apply_to(source, term, type.abstraction);
    case Type::Tag::tuple:
      return apply_to(source, term, type.tuple);
    case Type::Tag::destructured_tuple:
      return apply_to(source, term, type.destructured_tuple);
    case Type::Tag::subscript:
      return apply_to(source, term, type.subscript);
    case Type::Tag::list:
      return apply_to(source, term, type.list);
    case Type::Tag::assignment:
      return apply_to(source, term, type.assignment);
    default:
      MT_SHOW1("Unhandled apply to: ", source);
      assert(false);
      return source;
  }
}

void Unifier::apply_to(std::vector<TypeHandle>& sources, TermRef term) {
  for (auto& source : sources) {
    source = apply_to(source, term);
  }
}

TypeHandle Unifier::substitute_one(TypeRef source, TermRef term, TermRef lhs, TermRef rhs) {
  auto& type = store.at(source);

  switch (type.tag) {
    case Type::Tag::variable:
      return substitute_one(type.variable, source, term, lhs, rhs);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return substitute_one(type.abstraction, source, term, lhs, rhs);
    case Type::Tag::tuple:
      return substitute_one(type.tuple, source, term, lhs, rhs);
    case Type::Tag::destructured_tuple:
      return substitute_one(type.destructured_tuple, source, term, lhs, rhs);
    case Type::Tag::subscript:
      return substitute_one(type.subscript, source, term, lhs, rhs);
    case Type::Tag::list:
      return substitute_one(type.list, source, term, lhs, rhs);
    case Type::Tag::assignment:
      return substitute_one(type.assignment, source, term, lhs, rhs);
    default:
      MT_SHOW1("Unhandled: ", source);
      assert(false);
      return source;
  }
}

void Unifier::substitute_one(std::vector<TypeHandle>& sources, TermRef term, TermRef lhs, TermRef rhs) {
  for (auto& source : sources) {
    source = substitute_one(source, term, lhs, rhs);
  }
}

TypeHandle Unifier::substitute_one(types::Assignment& assignment, TypeRef source, TermRef term, TermRef lhs, TermRef rhs) {
  assignment.rhs = substitute_one(assignment.rhs, term, lhs, rhs);
  assignment.lhs = substitute_one(assignment.lhs, term, lhs, rhs);

  check_assignment(source, term, assignment);

  return source;
}

TypeHandle Unifier::substitute_one(types::Abstraction& func, TypeRef source, TermRef term, TermRef lhs, TermRef rhs) {
  func.inputs = substitute_one(func.inputs, term, lhs, rhs);
  func.outputs = substitute_one(func.outputs, term, lhs, rhs);

  check_push_func(source, term, func);

  return source;
}

TypeHandle Unifier::substitute_one(types::Tuple& tup, TypeRef source, TermRef term, TermRef lhs, TermRef rhs) {
  substitute_one(tup.members, term, lhs, rhs);
  return source;
}

TypeHandle Unifier::substitute_one(types::DestructuredTuple& tup, TypeRef source,
                                   TermRef term, TermRef lhs, TermRef rhs) {
  substitute_one(tup.members, term, lhs, rhs);
  return source;
}

TypeHandle Unifier::substitute_one(types::Variable& var, TypeRef source,
                                   TermRef term, TermRef lhs, TermRef rhs) {
  if (source == lhs.term) {
    return rhs.term;
  } else {
    return source;
  }
}

TypeHandle Unifier::substitute_one(types::Subscript& sub, TypeRef source,
                                   TermRef term, TermRef lhs, TermRef rhs) {
  sub.principal_argument = substitute_one(sub.principal_argument, term, lhs, rhs);

  for (auto& s : sub.subscripts) {
    substitute_one(s.arguments, term, lhs, rhs);
  }

  sub.outputs = substitute_one(sub.outputs, term, lhs, rhs);
  return maybe_unify_subscript(source, term, sub);
}

void Unifier::flatten_list(TypeRef source, std::vector<TypeHandle>& into) const {
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

TypeHandle Unifier::substitute_one(types::List& list, TypeRef source,
                                   TermRef term, TermRef lhs, TermRef rhs) {
  std::vector<TypeHandle> flattened;
  flatten_list(source, flattened);
  std::swap(list.pattern, flattened);

  TypeHandle last;
  int64_t remove_from = 1;
  int64_t num_remove = 0;

  for (int64_t i = 0; i < list.size(); i++) {
    auto& element = list.pattern[i];
    element = substitute_one(element, term, lhs, rhs);
    assert(element.is_valid());

    const bool should_remove = i > 0 && is_concrete_argument(element) &&
                               is_concrete_argument(last) &&
      TypeRelation(TypeEquivalence(), store).related_entry(element, last);

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

  eq.lhs.term = apply_to(eq.lhs.term, eq.lhs);
  eq.rhs.term = apply_to(eq.rhs.term, eq.rhs);

  if (eq.lhs.term == eq.rhs.term) {
    return;
  }

  const auto lhs_type = type_of(eq.lhs.term);
  const auto rhs_type = type_of(eq.rhs.term);

  if (lhs_type != Tag::variable && rhs_type != Tag::variable) {
    bool success = simplifier.simplify_entry(eq.lhs, eq.rhs);

    if (!success) {
      mark_failure();
//      MT_SHOW2("Failed to simplify: ", eq.lhs.term, eq.rhs.term);
//      assert(false);
    }

    return;

  } else if (lhs_type != Tag::variable) {
    assert(rhs_type == Tag::variable && "Rhs should be variable.");
    std::swap(eq.lhs, eq.rhs);
  }

  for (auto& subst_it : substitution->bound_variables) {
    auto& rhs_term = subst_it.second;
    rhs_term.term = substitute_one(rhs_term.term, rhs_term, eq.lhs, eq.rhs);
  }

  substitution->bound_variables[eq.lhs] = eq.rhs;
}

TypeHandle Unifier::maybe_unify_subscript(TypeRef source, TermRef term, types::Subscript& sub) {
  if (is_known_subscript_type(sub.principal_argument)) {
    return maybe_unify_known_subscript_type(source, term, sub);

  } else if (type_of(sub.principal_argument) == Type::Tag::abstraction) {
    //  a() where a is a variable, discovered to be a function reference.
    const auto& func = store.at(sub.principal_argument).abstraction;
    return maybe_unify_function_call_subscript(source, term, func, sub);

  } else if (type_of(sub.principal_argument) != Type::Tag::variable) {
    MT_SHOW1("Tried to unify subscript with principal arg: ", sub.principal_argument);
//    assert(false);
  }

  return source;
}

TypeHandle Unifier::maybe_unify_function_call_subscript(TypeRef source,
                                                        TermRef term,
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

  const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
  const auto sub_rhs_term = make_term(term.source_token, result_type);
  substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

  const auto arg_lhs_term = make_term(term.source_token, sub.principal_argument);
  const auto arg_rhs_term = make_term(term.source_token, lookup_handle);
  substitution->push_type_equation(make_eq(arg_lhs_term, arg_rhs_term));

  check_push_func(lookup_handle, term, store.at(lookup_handle).abstraction);
  registered_funcs[source] = true;

  return source;
}

TypeHandle Unifier::maybe_unify_known_subscript_type(TypeRef source, TermRef term, types::Subscript& sub) {
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
      const auto lhs_term = make_term(term.source_token, sub.outputs);
      const auto rhs_term = make_term(term.source_token, func.abstraction.outputs);
      substitution->push_type_equation(make_eq(lhs_term, rhs_term));

    } else {
      assert(func.tag == Type::Tag::scheme);
      const auto& func_scheme = func.scheme;
      const auto new_abstr_handle = instantiation.instantiate(func_scheme);
      const auto& func_ref = store.at(new_abstr_handle);
      assert(func_ref.is_abstraction());

      const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
      const auto sub_rhs_term = make_term(term.source_token, func_ref.abstraction.outputs);
      substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

      const auto tup_lhs_term = make_term(term.source_token, tup_type);
      const auto tup_rhs_term = make_term(term.source_token, func_ref.abstraction.inputs);
      substitution->push_type_equation(make_eq(tup_lhs_term, tup_rhs_term));
    }

    if (sub.subscripts.size() > 1) {
      auto result_type = store.make_fresh_type_variable_reference();

      const auto lhs_term = make_term(term.source_token, result_type);
      const auto rhs_term = make_term(term.source_token, sub.outputs);
      substitution->push_type_equation(make_eq(lhs_term, rhs_term));

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

void Unifier::emplace_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                             TypeRef lhs_type, TypeRef rhs_type) {
  add_error(make_simplification_failure(lhs_token, rhs_token, lhs_type, rhs_type));
}

SimplificationFailure Unifier::make_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                                           TypeRef lhs_type, TypeRef rhs_type) const {
  return SimplificationFailure(lhs_token, rhs_token, lhs_type, rhs_type);
}

void Unifier::add_error(SimplificationFailure&& err) {
  simplification_failures.emplace_back(std::move(err));
}

bool Unifier::had_error() const {
  return any_failures || !simplification_failures.empty();
}

void Unifier::mark_failure() {
  any_failures = true;
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
