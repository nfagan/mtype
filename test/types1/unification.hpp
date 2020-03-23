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

class TypeVisitor;
class DebugTypePrinter;
class Library;

struct UnifyResult {
  UnifyResult(std::vector<SimplificationFailure>&& simplify_failures) :
  simplify_failures(std::move(simplify_failures)), had_error(true) {
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
  std::vector<SimplificationFailure> simplify_failures;
};

class Unifier {
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

  void add_error(SimplificationFailure&& err);
  void emplace_simplification_failure(const Token* lhs_token, const Token* rhs_token,
                                      TypeRef lhs_type, TypeRef rhs_type);
  SimplificationFailure make_simplification_failure(const Token* lhs_token, const Token* rhs_token,
    TypeRef lhs_type, TypeRef rhs_type) const;

private:
  void reset(Substitution* subst);
  void unify_one(TypeEquation eq);

  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Abstraction& func);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Variable& var);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Tuple& tup);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::DestructuredTuple& tup);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Subscript& sub);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::List& list);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term, types::Assignment& assignment);
  MT_NODISCARD TypeHandle apply_to(TypeRef source, TermRef term);
  void apply_to(std::vector<TypeHandle>& sources, TermRef term);

  MT_NODISCARD TypeHandle substitute_one(TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  void substitute_one(std::vector<TypeHandle>& sources, TermRef term, TermRef lhs, TermRef rhs);

  MT_NODISCARD TypeHandle substitute_one(types::Variable& var,           TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Abstraction& func,       TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Tuple& tup,              TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::DestructuredTuple& tup,  TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Subscript& sub,          TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::List& list,              TypeRef source, TermRef term, TermRef lhs, TermRef rhs);
  MT_NODISCARD TypeHandle substitute_one(types::Assignment& assignment,  TypeRef source, TermRef term, TermRef lhs, TermRef rhs);

  Type::Tag type_of(TypeRef handle) const;
  void show();

  TypeHandle maybe_unify_subscript(TypeRef source, TermRef term, types::Subscript& sub);
  TypeHandle maybe_unify_known_subscript_type(TypeRef source, TermRef term, types::Subscript& sub);
  TypeHandle maybe_unify_function_call_subscript(TypeRef source, TermRef term,
    const types::Abstraction& source_func, types::Subscript& sub);

  void check_assignment(TypeRef source, TermRef term, const types::Assignment& assignment);
  void check_push_func(TypeRef source, TermRef term, const types::Abstraction& func);

  bool is_known_subscript_type(TypeRef handle) const;
  bool is_concrete_argument(TypeRef handle) const;
  bool are_concrete_arguments(const TypeHandles& handles) const;

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

  std::vector<SimplificationFailure> simplification_failures;
  bool any_failures;
};

}