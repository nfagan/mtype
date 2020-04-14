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
    const auto* candidate = search_result.external_function_candidate.value();
    const auto type = pending_external_functions->require_candidate_type(candidate, store);

    const auto lhs_term = make_term(term.source_token, source);
    const auto rhs_term = make_term(term.source_token, type);
    substitution->push_type_equation(make_eq(lhs_term, rhs_term));

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

Unifier::Unifier(TypeStore& store, const Library& library, StringRegistry& string_registry) :
  substitution(nullptr),
  pending_external_functions(nullptr),
  subtype_relationship(library),
  store(store),
  library(library),
  string_registry(string_registry),
  simplifier(*this, store),
  instantiation(store),
  subscript_handler(*this),
  any_failures(false) {
    //
}

void Unifier::reset(Substitution* subst, PendingExternalFunctions* external_functions) {
  assert(subst);
  assert(external_functions);

  substitution = subst;
  pending_external_functions = external_functions;
  errors.clear();
  any_failures = false;
}

UnifyResult Unifier::unify(Substitution* subst, PendingExternalFunctions* external_functions) {
  reset(subst, external_functions);

#if MT_REVERSE_UNIFY
  while (!substitution->type_equations.empty()) {
    auto eq = substitution->type_equations.back();
    substitution->type_equations.pop_back();
    unify_one(eq);
  }
#else
  while (substitution->equation_index < substitution->num_type_equations()) {
    unify_one(substitution->type_equations[substitution->equation_index++]);
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
    case Type::Tag::class_type:
      return occurs(MT_CLASS_REF(*type), term, lhs);
    case Type::Tag::record:
      return occurs(MT_RECORD_REF(*type), term, lhs);
    case Type::Tag::constant_value:
      return occurs(MT_CONST_VAL_REF(*type), term, lhs);
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

bool Unifier::occurs(const types::Class& class_type, TermRef term, const Type* lhs) const {
  return occurs(class_type.source, term, lhs);
}

bool Unifier::occurs(const types::Record& record, TermRef term, const Type* lhs) const {
  for (const auto& field : record.fields) {
    if (occurs(field.name, term, lhs)) {
      return true;
    } else if (occurs(field.type, term, lhs)) {
      return true;
    }
  }
  return false;
}

bool Unifier::occurs(const types::ConstantValue&, TermRef, const Type*) const {
  return false;
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

Type* Unifier::apply_to(types::Class& class_type, TermRef term) {
  class_type.source = apply_to(class_type.source, term);
  return &class_type;
}

Type* Unifier::apply_to(types::Record& record, TermRef term) {
  for (auto& field : record.fields) {
    field.name = apply_to(field.name, term);
    field.type = apply_to(field.type, term);
  }
  return &record;
}

Type* Unifier::apply_to(types::ConstantValue& val, TermRef) {
  //  Nothing to do yet.
  return &val;
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
    case Type::Tag::class_type:
      return apply_to(MT_CLASS_MUT_REF(*source), term);
    case Type::Tag::record:
      return apply_to(MT_RECORD_MUT_REF(*source), term);
    case Type::Tag::constant_value:
      return apply_to(MT_CONST_VAL_MUT_REF(*source), term);
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
    case Type::Tag::class_type:
      return substitute_one(MT_CLASS_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::record:
      return substitute_one(MT_RECORD_MUT_REF(*source), term, lhs, rhs);
    case Type::Tag::constant_value:
      return substitute_one(MT_CONST_VAL_MUT_REF(*source), term, lhs, rhs);
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

Type* Unifier::substitute_one(types::Class& class_type, TermRef term, TermRef lhs, TermRef rhs) {
  class_type.source = substitute_one(class_type.source, term, lhs, rhs);
  return &class_type;
}

Type* Unifier::substitute_one(types::Assignment& assignment, TermRef term, TermRef lhs, TermRef rhs) {
  assignment.rhs = substitute_one(assignment.rhs, term, lhs, rhs);
  assignment.lhs = substitute_one(assignment.lhs, term, lhs, rhs);

  check_assignment(&assignment, term, assignment);

  return &assignment;
}

Type* Unifier::substitute_one(types::Record& record, TermRef term, TermRef lhs, TermRef rhs) {
  for (auto& field : record.fields) {
    field.name = substitute_one(field.name, term, lhs, rhs);
    field.type = substitute_one(field.type, term, lhs, rhs);
  }
  return &record;
}

Type* Unifier::substitute_one(types::ConstantValue& val, TermRef, TermRef, TermRef) {
  //  Nothing to do yet.
  return &val;
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
  subscript_handler.maybe_unify_subscript(&sub, term, sub);

  return &sub;
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

DebugTypePrinter Unifier::type_printer() const {
  return DebugTypePrinter(&library, &string_registry);
}

void Unifier::emplace_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                             const Type* lhs_type, const Type* rhs_type) {
  add_error(make_simplification_failure(lhs_token, rhs_token, lhs_type, rhs_type));
}

BoxedTypeError Unifier::make_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                                    const Type* lhs_type, const Type* rhs_type) const {
  return std::make_unique<SimplificationFailure>(lhs_token, rhs_token, lhs_type, rhs_type);
}

BoxedTypeError Unifier::make_occurs_check_violation(const Token* lhs_token, const Token* rhs_token,
                                                    const Type* lhs_type, const Type* rhs_type) const {
  return std::make_unique<OccursCheckFailure>(lhs_token, rhs_token, lhs_type, rhs_type);
}

BoxedTypeError Unifier::make_unresolved_function_error(const Token* at_token, const Type* function_type) const {
  return std::make_unique<UnresolvedFunctionError>(at_token, function_type);
}

BoxedTypeError Unifier::make_invalid_function_invocation_error(const Token* at_token,
                                                               const Type* function_type) const {
  return std::make_unique<InvalidFunctionInvocationError>(at_token, function_type);
}

BoxedTypeError Unifier::make_non_constant_field_reference_expr_error(const Token* at_token,
                                                                     const Type* arg_type) const {
  return std::make_unique<NonConstantFieldReferenceExprError>(at_token, arg_type);
}

BoxedTypeError Unifier::make_reference_to_non_existent_field_error(const Token* at_token,
                                                                   const Type* arg,
                                                                   const Type* field) const {
  return std::make_unique<NonexistentFieldReferenceError>(at_token, arg, field);
}

BoxedTypeError Unifier::make_unhandled_custom_subscripts_error(const Token* at_token, const Type* principal_arg) const {
  return std::make_unique<UnhandledCustomSubscriptsError>(at_token, principal_arg);
}

void Unifier::add_error(BoxedTypeError err) {
  errors.emplace_back(std::move(err));
}

void Unifier::register_visited_type(Type* type) {
  registered_funcs[type] = true;
}

void Unifier::unregister_visited_type(Type* type) {
  registered_funcs.erase(type);
}

bool Unifier::is_visited_type(Type* type) const {
  return registered_funcs.count(type) > 0;
}

bool Unifier::had_error() const {
  return any_failures || !errors.empty();
}

void Unifier::mark_failure() {
  any_failures = true;
}

/*
 * PendingExternalFunctions
 */

void PendingExternalFunctions::add_candidate(const SearchCandidate* candidate, Type* type) {
  candidates[candidate] = type;
}

bool PendingExternalFunctions::has_candidate(const mt::SearchCandidate* candidate) {
  return candidates.count(candidate) > 0;
}

Type* PendingExternalFunctions::require_candidate_type(const SearchCandidate* candidate, TypeStore& store) {
  if (has_candidate(candidate)) {
    return candidates.at(candidate);
  } else {
    auto t = store.make_fresh_type_variable_reference();
    candidates[candidate] = t;
    return t;
  }
}

}
