#include "unification.hpp"
#include "type_constraint_gen.hpp"
#include "library.hpp"
#include "debug.hpp"
#include "type_properties.hpp"
#include "../string.hpp"
#include <algorithm>

#define MT_REVERSE_UNIFY (0)

#define MT_SHOW1(msg, a) \
  std::cout << (msg); \
  type_printer().show((a)); \
  std::cout << std::endl

#define MT_SHOW2(msg, a, b) \
  std::cout << (msg) << std::endl; \
  type_printer().show2((a), (b)); \
  std::cout << std::endl

namespace mt {

bool Unifier::is_concrete_argument(const Type* handle) const {
  TypeProperties props;
  return props.is_concrete_argument(handle);
}

bool Unifier::are_concrete_arguments(const mt::TypePtrs& handles) const {
  TypeProperties props;
  return props.are_concrete_arguments(handles);
}

bool Unifier::is_known_subscript_type(const Type* handle) const {
  return is_concrete_argument(handle) && library.is_known_subscript_type(handle);
}

void Unifier::check_push_function(Type* source, TermRef term, const types::Abstraction& func) {
  if (registered_funcs.count(source) > 0 || func.is_anonymous() || !is_concrete_argument(func.inputs)) {
    return;
  }

  assert(func.inputs->is_destructured_tuple());
  const auto search_result = library.search_function(func);

  if (search_result.resolved_type) {
    //  This function has a known type.
    const auto func_ref = search_result.resolved_type.value();

    if (func_ref->is_abstraction()) {
      const auto lhs_term = make_term(term.source_token, source);
      const auto rhs_term = make_term(term.source_token, func_ref);
      substitution->push_type_equation(make_eq(lhs_term, rhs_term));

    } else {
      assert(func_ref->is_scheme());
      const auto& func_scheme = MT_SCHEME_REF(*func_ref);
      const auto new_abstr_handle = instantiate(func_scheme);
      assert(new_abstr_handle->is_abstraction());

      const auto lhs_term = make_term(term.source_token, source);
      const auto rhs_term = make_term(term.source_token, new_abstr_handle);
      substitution->push_type_equation(make_eq(lhs_term, rhs_term));

      registered_funcs[new_abstr_handle] = true;
    }

  } else if (search_result.external_function_candidate) {
    //  This function was located in at least one file.

  } else {
    add_error(make_unresolved_function_error(term.source_token, source));
  }

  registered_funcs[source] = true;
}

void Unifier::check_assignment(Type* source, TermRef term, const types::Assignment& assignment) {
  if (registered_assignments.count(source) > 0) {
    return;
  }

  if (is_concrete_argument(assignment.rhs)) {
    //  In assignment lhs = rhs, rhs must be a subtype of lhs
    const auto lhs_term = make_term(term.source_token, assignment.rhs);
    const auto rhs_term = make_term(term.source_token, assignment.lhs);
    substitution->push_type_equation(make_eq(lhs_term, rhs_term));

    registered_assignments[source] = true;
  }
}

void Unifier::reset(Substitution* subst) {
  assert(subst);

  substitution = subst;
  errors.clear();
  any_failures = false;
}

UnifyResult Unifier::unify(Substitution* subst) {
  reset(subst);

#if MT_REVERSE_UNIFY
  while (!substitution->type_equations.empty()) {
    auto eq = substitution->type_equations.back();
    substitution->type_equations.pop_back();
    unify_one(eq);
  }
#else
  int64_t i = 0;
  while (i < substitution->num_type_equations()) {
    unify_one(substitution->type_equations[i++]);
  }
#endif

  if (had_error()) {
    return UnifyResult(std::move(errors));
  } else {
    return UnifyResult();
  }
}

bool Unifier::occurs(const Type* type, TermRef term, const Type* lhs) const {
  switch (type->tag) {
    case Type::Tag::abstraction:
      return occurs(MT_ABSTR_REF(*type), term, lhs);
    case Type::Tag::tuple:
      return occurs(MT_TUPLE_REF(*type), term, lhs);
    case Type::Tag::destructured_tuple:
      return occurs(MT_DT_REF(*type), term, lhs);
    case Type::Tag::subscript:
      return occurs(MT_SUBS_REF(*type), term, lhs);
    case Type::Tag::list:
      return occurs(MT_LIST_REF(*type), term, lhs);
    case Type::Tag::assignment:
      return occurs(MT_ASSIGN_REF(*type), term, lhs);
    case Type::Tag::scheme:
      return occurs(MT_SCHEME_REF(*type), term, lhs);
    case Type::Tag::scalar:
      return false;
    case Type::Tag::variable:
    case Type::Tag::parameters:
      return type == lhs;
    default:
      MT_SHOW1("Unhandled occurs: ", type);
      assert(false);
      return true;
  }
}

bool Unifier::occurs(const TypePtrs& ts, TermRef term, const Type* lhs) const {
  for (const auto& t : ts) {
    if (occurs(t, term, lhs)) {
      return true;
    }
  }
  return false;
}

bool Unifier::occurs(const types::Abstraction& abstr, TermRef term, const Type* lhs) const {
  return occurs(abstr.inputs, term, lhs) || occurs(abstr.outputs, term, lhs);
}

bool Unifier::occurs(const types::Tuple& tup, TermRef term, const Type* lhs) const {
  return occurs(tup.members, term, lhs);
}

bool Unifier::occurs(const types::DestructuredTuple& tup, TermRef term, const Type* lhs) const {
  return occurs(tup.members, term, lhs);
}

bool Unifier::occurs(const types::Subscript& sub, TermRef term, const Type* lhs) const {
  if (occurs(sub.outputs, term, lhs) || occurs(sub.principal_argument, term, lhs)) {
    return true;
  }
  for (const auto& s : sub.subscripts) {
    if (occurs(s.arguments, term, lhs)) {
      return true;
    }
  }
  return false;
}

bool Unifier::occurs(const types::List& list, TermRef term, const Type* lhs) const {
  return occurs(list.pattern, term, lhs);
}

bool Unifier::occurs(const types::Assignment& assignment, TermRef term, const Type* lhs) const {
  return occurs(assignment.lhs, term, lhs) || occurs(assignment.rhs, term, lhs);
}

bool Unifier::occurs(const types::Scheme& scheme, TermRef term, const Type* lhs) const {
  return occurs(scheme.type, term, lhs);
}

Type* Unifier::apply_to(types::Variable& var, TermRef) {
  TypeEquationTerm lookup(nullptr, &var);

  if (substitution->bound_terms.count(lookup) > 0) {
    return substitution->bound_terms.at(lookup).term;
  } else {
    return &var;
  }
}

Type* Unifier::apply_to(types::Parameters& params, TermRef) {
  if (expanded_parameters.count(&params) > 0) {
    return expanded_parameters.at(&params);
  } else {
    return &params;
  }
}

Type* Unifier::apply_to(types::Tuple& tup, TermRef term) {
  apply_to(tup.members, term);
  return &tup;
}

Type* Unifier::apply_to(types::DestructuredTuple& tup, TermRef term) {
  apply_to(tup.members, term);
  return &tup;
}

Type* Unifier::apply_to(types::Abstraction& func, TermRef term) {
  func.inputs = apply_to(func.inputs, term);
  func.outputs = apply_to(func.outputs, term);

  check_push_function(&func, term, func);

  return &func;
}

Type* Unifier::apply_to(types::Subscript& sub, TermRef term) {
  sub.principal_argument = apply_to(sub.principal_argument, term);
  for (auto& s : sub.subscripts) {
    apply_to(s.arguments, term);
  }
  sub.outputs = apply_to(sub.outputs, term);
  return &sub;
}

Type* Unifier::apply_to(types::List& list, TermRef term) {
  apply_to(list.pattern, term);
  return &list;
}

Type* Unifier::apply_to(types::Assignment& assignment, TermRef term) {
  assignment.lhs = apply_to(assignment.lhs, term);
  assignment.rhs = apply_to(assignment.rhs, term);

  check_assignment(&assignment, term, assignment);

  return &assignment;
}

Type* Unifier::apply_to(types::Scheme& scheme, TermRef term) {
  scheme.type = apply_to(scheme.type, term);
  return &scheme;
}

Type* Unifier::apply_to(Type* source, TermRef term) {
  switch (source->tag) {
    case Type::Tag::variable:
      return apply_to(MT_VAR_MUT_REF(*source), term);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return apply_to(MT_ABSTR_MUT_REF(*source), term);
    case Type::Tag::tuple:
      return apply_to(MT_TUPLE_MUT_REF(*source), term);
    case Type::Tag::destructured_tuple:
      return apply_to(MT_DT_MUT_REF(*source), term);
    case Type::Tag::subscript:
      return apply_to(MT_SUBS_MUT_REF(*source), term);
    case Type::Tag::list:
      return apply_to(MT_LIST_MUT_REF(*source), term);
    case Type::Tag::assignment:
      return apply_to(MT_ASSIGN_MUT_REF(*source), term);
    case Type::Tag::scheme:
      return apply_to(MT_SCHEME_MUT_REF(*source), term);
    case Type::Tag::parameters:
      return apply_to(MT_PARAMS_MUT_REF(*source), term);
    default:
      MT_SHOW1("Unhandled apply to: ", source);
      assert(false);
      return source;
  }
}

void Unifier::apply_to(TypePtrs& sources, TermRef term) {
  for (auto& source : sources) {
    source = apply_to(source, term);
  }
}

Type* Unifier::substitute_one(Type* source, TermRef term, TermRef lhs, TermRef rhs) {
  switch (source->tag) {
    case Type::Tag::variable:
      return substitute_one(MT_VAR_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::parameters:
      return substitute_one(MT_PARAMS_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::scalar:
      return source;
    case Type::Tag::abstraction:
      return substitute_one(MT_ABSTR_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::tuple:
      return substitute_one(MT_TUPLE_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::destructured_tuple:
      return substitute_one(MT_DT_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::subscript:
      return substitute_one(MT_SUBS_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::list:
      return substitute_one(MT_LIST_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::assignment:
      return substitute_one(MT_ASSIGN_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::scheme:
      return substitute_one(MT_SCHEME_MUT_REF(*source), term, lhs, rhs);
    default:
      MT_SHOW1("Unhandled: ", source);
      assert(false);
      return source;
  }
}

void Unifier::substitute_one(TypePtrs& sources, TermRef term, TermRef lhs, TermRef rhs) {
  for (auto& source : sources) {
    source = substitute_one(source, term, lhs, rhs);
  }
}

Type* Unifier::substitute_one(types::Scheme& scheme, TermRef term, TermRef lhs, TermRef rhs) {
  scheme.type = substitute_one(scheme.type, term, lhs, rhs);
  return &scheme;
}

Type* Unifier::substitute_one(types::Assignment& assignment, TermRef term, TermRef lhs, TermRef rhs) {
  assignment.rhs = substitute_one(assignment.rhs, term, lhs, rhs);
  assignment.lhs = substitute_one(assignment.lhs, term, lhs, rhs);

  check_assignment(&assignment, term, assignment);

  return &assignment;
}

Type* Unifier::substitute_one(types::Abstraction& func, TermRef term, TermRef lhs, TermRef rhs) {
  func.inputs = substitute_one(func.inputs, term, lhs, rhs);
  func.outputs = substitute_one(func.outputs, term, lhs, rhs);

  check_push_function(&func, term, func);

  return &func;
}

Type* Unifier::substitute_one(types::Tuple& tup, TermRef term, TermRef lhs, TermRef rhs) {
  substitute_one(tup.members, term, lhs, rhs);
  return &tup;
}

Type* Unifier::substitute_one(types::DestructuredTuple& tup, TermRef term, TermRef lhs, TermRef rhs) {
  substitute_one(tup.members, term, lhs, rhs);
  return &tup;
}

Type* Unifier::substitute_one(types::Variable& var, TermRef, TermRef lhs, TermRef rhs) {
  auto source = &var;

  if (source == lhs.term) {
    return rhs.term;
  } else {
    return source;
  }
}

Type* Unifier::substitute_one(types::Parameters& params, TermRef, TermRef, TermRef) {
  auto source = &params;

  if (expanded_parameters.count(source) > 0) {
    return expanded_parameters.at(source);
  } else {
    return source;
  }
}

Type* Unifier::substitute_one(types::Subscript& sub, TermRef term, TermRef lhs, TermRef rhs) {
  sub.principal_argument = substitute_one(sub.principal_argument, term, lhs, rhs);

  for (auto& s : sub.subscripts) {
    substitute_one(s.arguments, term, lhs, rhs);
  }

  sub.outputs = substitute_one(sub.outputs, term, lhs, rhs);
  return maybe_unify_subscript(&sub, term, sub);
}

void Unifier::flatten_list(Type* source, TypePtrs& into) const {
  switch (source->tag) {
    case Type::Tag::list: {
      const auto& list = MT_LIST_REF(*source);
      auto sz = list.size();
      for (int64_t i = 0; i < sz; i++) {
        flatten_list(list.pattern[i], into);
      }
      break;
    }
    case Type::Tag::destructured_tuple: {
      const auto& tup = MT_DT_REF(*source);
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

Type* Unifier::substitute_one(types::List& list, TermRef term, TermRef lhs, TermRef rhs) {
  TypePtrs flattened;
  flatten_list(&list, flattened);
  std::swap(list.pattern, flattened);

  Type* last = nullptr;
  int64_t remove_from = 1;
  int64_t num_remove = 0;

  for (int64_t i = 0; i < list.size(); i++) {
    auto& element = list.pattern[i];
    element = substitute_one(element, term, lhs, rhs);
    assert(element);

    const bool should_remove = i > 0 && is_concrete_argument(element) &&
                               is_concrete_argument(last) &&
      TypeRelation(EquivalenceRelation(), store).related_entry(element, last);

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

  return &list;
}

void Unifier::unify_one(TypeEquation eq) {
  using Tag = Type::Tag;

  auto lhs = eq.lhs;
  auto rhs = eq.rhs;

  lhs.term = apply_to(lhs.term, lhs);
  rhs.term = apply_to(rhs.term, rhs);

  if (lhs.term == rhs.term) {
    return;
  }

  auto lhs_type = lhs.term->tag;
  auto rhs_type = rhs.term->tag;

  if (lhs_type != Tag::variable && rhs_type != Tag::variable) {
    const bool success = simplifier.simplify_entry(lhs, rhs);
    if (!success) {
      mark_failure();
    }

    return;
  } else if (lhs_type != Tag::variable) {
    assert(rhs_type == Tag::variable && "Rhs should be variable.");
    std::swap(lhs, rhs);
    std::swap(lhs_type, rhs_type);
  }

  assert(lhs_type == Type::Tag::variable);

  if (occurs(rhs.term, rhs, lhs.term)) {
    add_error(make_occurs_check_violation(lhs.source_token, rhs.source_token, lhs.term, rhs.term));
    return;
  }

  for (auto& subst_it : substitution->bound_terms) {
    auto& rhs_term = subst_it.second;
    rhs_term.term = substitute_one(rhs_term.term, rhs_term, lhs, rhs);
  }

  substitution->bound_terms[lhs] = rhs;
}

Type* Unifier::instantiate(const types::Scheme& scheme) {
  auto instance_vars = instantiation.make_instance_variables(scheme);
  const auto instance_handle = instantiation.instantiate(scheme, instance_vars);
  auto instance_constraints = scheme.constraints;

  for (auto& constraint : instance_constraints) {
    constraint.lhs.term = instantiation.clone(constraint.lhs.term, instance_vars);
    constraint.rhs.term = instantiation.clone(constraint.rhs.term, instance_vars);

    if (constraint.lhs != constraint.rhs) {
      substitution->push_type_equation(constraint);
    }
  }

  return instance_handle;
}

Type* Unifier::maybe_unify_subscript(Type* source, TermRef term, types::Subscript& sub) {
  const auto arg0 = sub.principal_argument;

  if (is_known_subscript_type(sub.principal_argument)) {
    return maybe_unify_known_subscript_type(source, term, sub);

  } else if (arg0->is_abstraction()) {
    //  a() where a is a variable, discovered to be a function reference.
    return maybe_unify_function_call_subscript(source, term, MT_ABSTR_REF(*arg0), sub);

  } else if (arg0->is_scheme()) {
    return maybe_unify_anonymous_function_call_subscript(source, term, MT_SCHEME_REF(*arg0), sub);

  } else if (!arg0->is_variable() && !arg0->is_parameters()) {
    MT_SHOW1("Tried to unify subscript with principal arg: ", sub.principal_argument);
  }

  return source;
}

Type* Unifier::maybe_unify_anonymous_function_call_subscript(Type* source,
                                                             TermRef term,
                                                             const types::Scheme& scheme,
                                                             types::Subscript& sub) {
  if (!scheme.type->is_abstraction() || registered_funcs.count(source) > 0) {
    return source;
  }

  if (sub.subscripts.size() != 1 || sub.subscripts[0].method != SubscriptMethod::parens) {
    registered_funcs[source] = true;
    add_error(make_invalid_function_invocation_error(term.source_token, sub.principal_argument));
    return source;
  }

  const auto& sub0 = sub.subscripts[0];

  if (!are_concrete_arguments(sub0.arguments)) {
    return source;
  }

  const auto instance_handle = instantiate(scheme);
  assert(instance_handle->is_abstraction());
  const auto& source_func = MT_ABSTR_REF(*instance_handle);

  const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
  const auto sub_rhs_term = make_term(term.source_token, source_func.outputs);
  substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

  auto copy_args = sub0.arguments;
  auto new_inputs = store.make_rvalue_destructured_tuple(std::move(copy_args));
  const auto arg_lhs_term = make_term(term.source_token, new_inputs);
  const auto arg_rhs_term = make_term(term.source_token, source_func.inputs);
  substitution->push_type_equation(make_eq(arg_lhs_term, arg_rhs_term));

  registered_funcs[source] = true;
  return source;
}

Type* Unifier::maybe_unify_function_call_subscript(Type* source,
                                                   TermRef term,
                                                   const types::Abstraction& source_func,
                                                   const types::Subscript& sub) {
  if (registered_funcs.count(source) > 0) {
    return source;
  }

  if (sub.subscripts.size() != 1 || sub.subscripts[0].method != SubscriptMethod::parens) {
    registered_funcs[source] = true;
    add_error(make_invalid_function_invocation_error(term.source_token, sub.principal_argument));
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
  auto lookup_handle = store.make_abstraction(std::move(lookup));

  const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
  const auto sub_rhs_term = make_term(term.source_token, result_type);
  substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

  const auto arg_lhs_term = make_term(term.source_token, sub.principal_argument);
  const auto arg_rhs_term = make_term(term.source_token, lookup_handle);
  substitution->push_type_equation(make_eq(arg_lhs_term, arg_rhs_term));

  check_push_function(lookup_handle, term, MT_ABSTR_REF(*lookup_handle));
  registered_funcs[source] = true;

  return source;
}

Type* Unifier::maybe_unify_known_subscript_type(Type* source, TermRef term, types::Subscript& sub) {
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

  TypePtrs into;
  auto args = sub0.arguments;
  args.insert(args.begin(), sub.principal_argument);

  DestructuredTuple tup(DestructuredTuple::Usage::rvalue, std::move(args));

  auto tup_type = store.make_destructured_tuple(std::move(tup));
  Abstraction abstraction(sub0.method, tup_type, nullptr);

  auto maybe_func = library.lookup_function(abstraction);
  if (maybe_func) {
//    MT_SHOW1("OK: found subscript signature: ", tup_type);
    const auto func = maybe_func.value();

    if (func->is_abstraction()) {
      const auto lhs_term = make_term(term.source_token, sub.outputs);
      const auto rhs_term = make_term(term.source_token, MT_ABSTR_REF(*func).outputs);
      substitution->push_type_equation(make_eq(lhs_term, rhs_term));

    } else {
      assert(func->is_scheme());
      const auto& func_scheme = MT_SCHEME_REF(*func);
      const auto func_ref = instantiation.instantiate(func_scheme);
      assert(func_ref->is_abstraction());

      const auto sub_lhs_term = make_term(term.source_token, sub.outputs);
      const auto sub_rhs_term = make_term(term.source_token, MT_ABSTR_REF(*func_ref).outputs);
      substitution->push_type_equation(make_eq(sub_lhs_term, sub_rhs_term));

      const auto tup_lhs_term = make_term(term.source_token, tup_type);
      const auto tup_rhs_term = make_term(term.source_token, MT_ABSTR_REF(*func_ref).inputs);
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
    add_error(make_unresolved_function_error(term.source_token, source));
    registered_funcs[source] = true;
  }

  return source;
}

DebugTypePrinter Unifier::type_printer() const {
  return DebugTypePrinter(&library, &string_registry);
}

void Unifier::emplace_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                             const Type* lhs_type, const Type* rhs_type) {
  add_error(make_simplification_failure(lhs_token, rhs_token, lhs_type, rhs_type));
}

BoxedUnificationError Unifier::make_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                                           const Type* lhs_type, const Type* rhs_type) const {
  return std::make_unique<SimplificationFailure>(lhs_token, rhs_token, lhs_type, rhs_type);
}

BoxedUnificationError Unifier::make_occurs_check_violation(const Token* lhs_token, const Token* rhs_token,
                                                           const Type* lhs_type, const Type* rhs_type) const {
  return std::make_unique<OccursCheckFailure>(lhs_token, rhs_token, lhs_type, rhs_type);
}

BoxedUnificationError Unifier::make_unresolved_function_error(const Token* at_token, const Type* function_type) const {
  return std::make_unique<UnresolvedFunctionError>(at_token, function_type);
}

BoxedUnificationError Unifier::make_invalid_function_invocation_error(const Token* at_token,
                                                                      const Type* function_type) const {
  return std::make_unique<InvalidFunctionInvocationError>(at_token, function_type);
}

void Unifier::add_error(BoxedUnificationError err) {
  errors.emplace_back(std::move(err));
}

bool Unifier::had_error() const {
  return any_failures || !errors.empty();
}

void Unifier::mark_failure() {
  any_failures = true;
}

}