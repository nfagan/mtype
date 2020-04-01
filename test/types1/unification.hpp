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
  errors(std::move(errors)), had_error(true) {
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
  using BoundVariables = std::unordered_map<TypeEquationTerm, TypeEquationTerm, TypeEquationTerm::HandleHash>;
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
  DebugTypePrinter type_printer() const;

  void add_error(BoxedUnificationError err);
  void emplace_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                      TypeRef lhs_type, TypeRef rhs_type);
  BoxedUnificationError make_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                                    TypeRef lhs_type, TypeRef rhs_type) const;
  BoxedUnificationError make_occurs_check_violation(const Token* lhs_token, const Token* rhs_token,
                                                    TypeRef lhs_type, TypeRef rhs_type) const;
  BoxedUnificationError make_unresolved_function_error(const Token* at_token, TypeRef function_type) const;
  BoxedUnificationError make_invalid_function_invocation_error(const Token* at_token, TypeRef function_type) const;

private:
  void reset(Substitution* subst);
  void unify_one(TypeEquation eq);

  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Abstraction& func);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Variable& var);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Parameters& params);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Tuple& tup);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::DestructuredTuple& tup);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Subscript& sub);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::List& list);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Assignment& assignment);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Scheme& scheme);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term);
  void apply_to(std::vector<TypeHandle>& sources, TermRef term);

  MT_NODISCARD TypeHandle substitute_one(TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  void substitute_one(std::vector<TypeHandle>& sources, TermRef term, TermRef lhs, TermRef rhs);

  MT_NODISCARD TypeHandle substitute_one(types::Variable& var,           TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Parameters& params,      TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Abstraction& func,       TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Tuple& tup,              TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::DestructuredTuple& tup,  TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Subscript& sub,          TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::List& list,              TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Assignment& assignment,  TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Scheme& scheme,          TypeRef source, TermRef term, TermRef lhs, TermRef rhs);

  bool occurs(TypeRef t, TermRef term, TypeRef lhs) const;
  bool occurs(const TypeHandles& ts, TermRef term, TypeRef lhs) const;
  bool occurs(const types::Abstraction& abstr,      TermRef term, TypeRef lhs) const;
  bool occurs(const types::Tuple& tup,              TermRef term, TypeRef lhs) const;
  bool occurs(const types::DestructuredTuple& tup,  TermRef term, TypeRef lhs) const;
  bool occurs(const types::Subscript& sub,          TermRef term, TypeRef lhs) const;
  bool occurs(const types::List& list,              TermRef term, TypeRef lhs) const;
  bool occurs(const types::Assignment& assignment,  TermRef term, TypeRef lhs) const;
  bool occurs(const types::Scheme& scheme,          TermRef term, TypeRef lhs) const;

  Type::Tag type_of(TypeRef handle) const;
  void show();

  TypeHandle maybe_unify_subscript(TypeRef source, TermRef term, types::Subscript& sub);
  TypeHandle maybe_unify_known_subscript_type(TypeRef source, TermRef term, types::Subscript& sub);
  TypeHandle maybe_unify_function_call_subscript(TypeRef source, TermRef term,
    const types::Abstraction& source_func, const types::Subscript& sub);
  TypeHandle maybe_unify_anonymous_function_call_subscript(TypeRef source, TermRef term,
    const types::Scheme& scheme, types::Subscript& sub);

  void check_assignment(TypeRef source, TermRef term, const types::Assignment& assignment);
  void check_push_function(TypeRef source, TermRef term, const types::Abstraction& func);

  bool is_known_subscript_type(TypeRef handle) const;
  bool is_concrete_argument(TypeRef handle) const;
  bool are_concrete_arguments(const TypeHandles& handles) const;

  TypeHandle instantiate(const types::Scheme& scheme);

  void flatten_list(TypeRef source, std::vector<TypeHandle>& into) const;
  bool had_error() const;
  void mark_failure();

public:
  Substitution* substitution;
  SubtypeRelation subtype_relationship;

private:
  TypeStore& store;
  const Library& library;
  StringRegistry& string_registry;
  Simplifier simplifier;
  Instantiation instantiation;

  std::unordered_map<TypeHandle, bool, TypeHandle::Hash> registered_funcs;
  std::unordered_map<TypeHandle, bool, TypeHandle::Hash> registered_assignments;
  std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash> expanded_parameters;

  UnificationErrors errors;
  bool any_failures;
};

}