#pragma once

#include "mt/mt.hpp"
#include "type.hpp"
#include "type_store.hpp"
#include "instance.hpp"
#include "simplify.hpp"
#include "error.hpp"
#include "substitution.hpp"
#include "type_relationships.hpp"
#include <map>

namespace mt {

class TypeConstraintGenerator;
class DebugTypePrinter;
class Library;

struct UnifyResult {
  UnifyResult(UnificationErrors&& errors) :
  had_error(true), errors(std::move(errors)) {
    //
  }

  UnifyResult() : had_error(false) {
    //
  }

  bool is_success() const {
    return !had_error;
  }

  bool is_error() const {
    return had_error;
  }

  bool had_error;
  UnificationErrors errors;
};

class Unifier {
  friend class Simplifier;
public:
  using BoundVariables = std::unordered_map<TypeEquationTerm, TypeEquationTerm, TypeEquationTerm::TypeHash>;
public:
  Unifier(TypeStore& store, const Library& library, StringRegistry& string_registry) :
  substitution(nullptr),
  subtype_relationship(library),
  store(store),
  library(library),
  string_registry(string_registry),
  simplifier(*this, store),
  instantiation(store),
  any_failures(false) {
    //
  }

  MT_NODISCARD UnifyResult unify(Substitution* subst);

private:
  void reset(Substitution* subst);
  void unify_one(TypeEquation eq);

  MT_NODISCARD Type* apply_to(types::Abstraction& func, TermRef term);
  MT_NODISCARD Type* apply_to(types::Variable& var, TermRef term);
  MT_NODISCARD Type* apply_to(types::Parameters& params, TermRef term);
  MT_NODISCARD Type* apply_to(types::Tuple& tup, TermRef term);
  MT_NODISCARD Type* apply_to(types::DestructuredTuple& tup, TermRef term);
  MT_NODISCARD Type* apply_to(types::Subscript& sub, TermRef term);
  MT_NODISCARD Type* apply_to(types::List& list, TermRef term);
  MT_NODISCARD Type* apply_to(types::Assignment& assignment, TermRef term);
  MT_NODISCARD Type* apply_to(types::Scheme& scheme, TermRef term);
  MT_NODISCARD Type* apply_to(Type* source, TermRef term);
  void apply_to(TypePtrs& sources, TermRef term);

  MT_NODISCARD Type* substitute_one(Type* source, TermRef term, TermRef lhs, TermRef rhs);
  void substitute_one(TypePtrs& sources, TermRef term, TermRef lhs, TermRef rhs);

  MT_NODISCARD Type* substitute_one(types::Variable& var, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Parameters& params, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Abstraction& func, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Tuple& tup, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::DestructuredTuple& tup, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Subscript& sub, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::List& list, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Assignment& assignment, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Scheme& scheme, TermRef term, TermRef lhs, TermRef rhs);

  bool occurs(const Type* t, TermRef term, const Type* lhs) const;
  bool occurs(const TypePtrs& ts, TermRef term, const Type* lhs) const;
  bool occurs(const types::Abstraction& abstr, TermRef term, const Type* lhs) const;
  bool occurs(const types::Tuple& tup, TermRef term, const Type* lhs) const;
  bool occurs(const types::DestructuredTuple& tup, TermRef term, const Type* lhs) const;
  bool occurs(const types::Subscript& sub, TermRef term, const Type* lhs) const;
  bool occurs(const types::List& list, TermRef term, const Type* lhs) const;
  bool occurs(const types::Assignment& assignment, TermRef term, const Type* lhs) const;
  bool occurs(const types::Scheme& scheme, TermRef term, const Type* lhs) const;

  Type* maybe_unify_subscript(Type* source, TermRef term, types::Subscript& sub);
  Type* maybe_unify_known_subscript_type(Type* source, TermRef term, types::Subscript& sub);
  Type* maybe_unify_function_call_subscript(Type* source, TermRef term,
    const types::Abstraction& source_func, const types::Subscript& sub);
  Type* maybe_unify_anonymous_function_call_subscript(Type* source, TermRef term,
    const types::Scheme& scheme, types::Subscript& sub);

  void check_assignment(Type* source, TermRef term, const types::Assignment& assignment);
  void check_push_function(Type* source, TermRef term, const types::Abstraction& func);

  bool is_known_subscript_type(const Type* handle) const;
  bool is_concrete_argument(const Type* handle) const;
  bool are_concrete_arguments(const TypePtrs& handles) const;

  Type* instantiate(const types::Scheme& scheme);

  DebugTypePrinter type_printer() const;

  void flatten_list(Type* source, TypePtrs& into) const;
  bool had_error() const;
  void mark_failure();

  void add_error(BoxedUnificationError err);
  void emplace_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                      const Type* lhs_type, const Type* rhs_type);
  BoxedUnificationError make_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                                    const Type* lhs_type, const Type* rhs_type) const;
  BoxedUnificationError make_occurs_check_violation(const Token* lhs_token, const Token* rhs_token,
                                                    const Type* lhs_type, const Type* rhs_type) const;
  BoxedUnificationError make_unresolved_function_error(const Token* at_token, const Type* function_type) const;
  BoxedUnificationError make_invalid_function_invocation_error(const Token* at_token, const Type* function_type) const;

private:
  Substitution* substitution;
  SubtypeRelation subtype_relationship;

  TypeStore& store;
  const Library& library;
  StringRegistry& string_registry;
  Simplifier simplifier;
  Instantiation instantiation;

  std::unordered_map<Type*, bool> registered_funcs;
  std::unordered_map<Type*, bool> registered_assignments;
  std::unordered_map<Type*, Type*> expanded_parameters;

  UnificationErrors errors;
  bool any_failures;
};

}