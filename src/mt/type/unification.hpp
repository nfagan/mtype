#pragma once

#include "types.hpp"
#include "instance.hpp"
#include "simplify.hpp"
#include "subscripts.hpp"
#include "error.hpp"
#include "substitution.hpp"
#include "type_relationships.hpp"
#include <map>

namespace mt {

class DebugTypePrinter;
class Library;
class StringRegistry;
class TypeStore;
struct SearchCandidate;

/*
 * PendingExternalFunctions
 */

struct PendingExternalFunctions {
  using PendingCandidates = std::unordered_map<const SearchCandidate*, Type*>;
  PendingExternalFunctions() = default;

  bool has_candidate(const SearchCandidate* candidate) const;
  void add_candidate(const SearchCandidate* candidate, Type* with_type);
  Type* require_candidate_type(const SearchCandidate* candidate, TypeStore& store);
  //
  PendingCandidates candidates;
};

/*
 * UnifyResult
 */

struct UnifyResult {
  explicit UnifyResult(TypeErrors&& errors) :
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
  TypeErrors errors;
};

/*
 * Unifier
 */

class Unifier {
  friend class Simplifier;
  friend class SubscriptHandler;
public:
  using BoundVariables = std::unordered_map<TypeEquationTerm, TypeEquationTerm, TypeEquationTerm::TypeHash>;
public:
  Unifier(TypeStore& store, const Library& library, StringRegistry& string_registry);
  MT_NODISCARD UnifyResult unify(Substitution* subst, PendingExternalFunctions* external_functions);

private:
  void reset(Substitution* subst, PendingExternalFunctions* external_functions);
  void unify_one(TypeEquation eq);

  MT_NODISCARD Type* apply_to(types::Abstraction& func, TermRef term);
  MT_NODISCARD Type* apply_to(types::Application& app, TermRef term);
  MT_NODISCARD Type* apply_to(types::Variable& var, TermRef term);
  MT_NODISCARD Type* apply_to(types::Parameters& params, TermRef term);
  MT_NODISCARD Type* apply_to(types::Tuple& tup, TermRef term);
  MT_NODISCARD Type* apply_to(types::Union& union_type, TermRef term);
  MT_NODISCARD Type* apply_to(types::DestructuredTuple& tup, TermRef term);
  MT_NODISCARD Type* apply_to(types::Subscript& sub, TermRef term);
  MT_NODISCARD Type* apply_to(types::List& list, TermRef term);
  MT_NODISCARD Type* apply_to(types::Assignment& assignment, TermRef term);
  MT_NODISCARD Type* apply_to(types::Scheme& scheme, TermRef term);
  MT_NODISCARD Type* apply_to(types::Class& class_type, TermRef term);
  MT_NODISCARD Type* apply_to(types::Record& record, TermRef term);
  MT_NODISCARD Type* apply_to(types::ConstantValue& val, TermRef term);
  MT_NODISCARD Type* apply_to(types::Alias& alias, TermRef term);
  MT_NODISCARD Type* apply_to(Type* source, TermRef term);
  void apply_to(TypePtrs& sources, TermRef term);

  MT_NODISCARD Type* substitute_one(Type* source, TermRef term, TermRef lhs, TermRef rhs);
  void substitute_one(TypePtrs& sources, TermRef term, TermRef lhs, TermRef rhs);

  MT_NODISCARD Type* substitute_one(types::Variable& var, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Parameters& params, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Abstraction& func, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Application& app, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Tuple& tup, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Union& union_type, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::DestructuredTuple& tup, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Subscript& sub, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::List& list, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Assignment& assignment, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Scheme& scheme, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Class& class_type, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Record& record, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::ConstantValue& val, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD Type* substitute_one(types::Alias& val, TermRef term, TermRef lhs, TermRef rhs);

  bool occurs(const Type* t, TermRef term, const Type* lhs) const;
  bool occurs(const TypePtrs& ts, TermRef term, const Type* lhs) const;
  bool occurs(const types::Abstraction& abstr, TermRef term, const Type* lhs) const;
  bool occurs(const types::Application& app, TermRef term, const Type* lhs) const;
  bool occurs(const types::Tuple& tup, TermRef term, const Type* lhs) const;
  bool occurs(const types::Union& union_type, TermRef term, const Type* lhs) const;
  bool occurs(const types::DestructuredTuple& tup, TermRef term, const Type* lhs) const;
  bool occurs(const types::Subscript& sub, TermRef term, const Type* lhs) const;
  bool occurs(const types::List& list, TermRef term, const Type* lhs) const;
  bool occurs(const types::Assignment& assignment, TermRef term, const Type* lhs) const;
  bool occurs(const types::Scheme& scheme, TermRef term, const Type* lhs) const;
  bool occurs(const types::Class& class_type, TermRef term, const Type* lhs) const;
  bool occurs(const types::Record& record, TermRef term, const Type* lhs) const;
  bool occurs(const types::ConstantValue& val, TermRef term, const Type* lhs) const;
  bool occurs(const types::Alias& alias, TermRef term, const Type* lhs) const;

  void check_assignment(Type* source, TermRef term, const types::Assignment& assignment);
  void check_application(Type* source, TermRef term, const types::Application& app);

  bool is_concrete_argument(const Type* handle) const;
  bool are_concrete_arguments(const TypePtrs& handles) const;

  Type* instantiate(const types::Scheme& scheme);

  DebugTypePrinter type_printer() const;

  void flatten_list(Type* source, TypePtrs& into) const;
  bool had_error() const;
  void mark_failure();

  void register_visited_type(Type* type);
  void unregister_visited_type(Type* type);
  bool is_visited_type(Type* type) const;

  void add_error(BoxedTypeError err);
  void emplace_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                      const Type* lhs_type, const Type* rhs_type);
  BoxedTypeError make_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                             const Type* lhs_type, const Type* rhs_type) const;
  BoxedTypeError make_occurs_check_violation(const Token* lhs_token, const Token* rhs_token,
                                             const Type* lhs_type, const Type* rhs_type) const;
  BoxedTypeError make_unresolved_function_error(const Token* at_token,
                                                const Type* function_type) const;
  BoxedTypeError make_invalid_function_invocation_error(const Token* at_token, const Type* function_type) const;
  BoxedTypeError make_non_constant_field_reference_expr_error(const Token* at_token, const Type* arg_type) const;
  BoxedTypeError make_reference_to_non_existent_field_error(const Token* at_token, const Type* arg, const Type* field) const;
  BoxedTypeError make_unhandled_custom_subscripts_error(const Token* at_token, const Type* principal_arg) const;

private:
  Substitution* substitution;
  PendingExternalFunctions* pending_external_functions;
  SubtypeRelation subtype_relationship;

  TypeStore& store;
  const Library& library;
  StringRegistry& string_registry;
  Simplifier simplifier;
  Instantiation instantiation;
  SubscriptHandler subscript_handler;

  std::unordered_map<Type*, bool> registered_funcs;
  std::unordered_map<Type*, bool> registered_assignments;
  std::unordered_map<Type*, Type*> expanded_parameters;

  TypeErrors errors;
  bool any_failures;
};

}