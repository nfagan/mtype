#pragma once

#include "mt/mt.hpp"
#include "type.hpp"
#include "type_equality.hpp"
#include "type_store.hpp"
#include "instance.hpp"
#include "simplify.hpp"
#include <map>

namespace mt {

class TypeVisitor;
class DebugTypePrinter;
class Library;

class Unifier {
  friend class Simplifier;
public:
  using BoundVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;
public:
  Unifier(TypeStore& store, const Library& library, StringRegistry& string_registry) :
  store(store),
  library(library),
  string_registry(string_registry),
  simplifier(*this, store),
  instantiation(store) {
    //
  }

  ~Unifier() {
    std::cout << "Num type eqs: " << type_equations.size() << std::endl;
    std::cout << "Subs size: " << bound_variables.size() << std::endl;
    std::cout << "Num types: " << store.size() << std::endl;
  }

  void push_type_equation(TypeEquation&& eq) {
    type_equations.emplace_back(std::move(eq));
  }

  void unify();
  Optional<TypeHandle> bound_type(const TypeHandle& for_type) const;

private:
  using Term = TypeEquationTerm;

  void unify_one(TypeEquation eq);

  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term, types::Abstraction& func);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term, types::Variable& var);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term, types::Tuple& tup);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term, types::DestructuredTuple& tup);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term, types::Subscript& sub);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term, types::List& list);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term, types::Assignment& assignment);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, const TypeEquationTerm& term);
  void apply_to(std::vector<TypeHandle>& sources, const TypeEquationTerm& term);

  [[nodiscard]] TypeHandle substitute_one(types::Variable& var, const TypeHandle& source, const Term& lhs, const Term& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::Abstraction& func, const TypeHandle& source, const Term& lhs, const Term& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::Tuple& tup, const TypeHandle& source, const Term& lhs, const Term& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::DestructuredTuple& tup, const TypeHandle& source, const Term& lhs, const Term& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::Subscript& sub, const TypeHandle& source, const Term& lhs, const Term& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::List& list, const TypeHandle& source, const Term& lhs, const Term& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::Assignment& assignment, const TypeHandle& source, const Term& lhs, const Term& rhs);
  [[nodiscard]] TypeHandle substitute_one(const TypeHandle& source, const Term& lhs, const Term& rhs);
  void substitute_one(std::vector<TypeHandle>& sources, const Term& lhs, const Term& rhs);

  Type::Tag type_of(const TypeHandle& handle) const;
  void show();

  TypeHandle maybe_unify_subscript(const TypeHandle& source, types::Subscript& sub);
  TypeHandle maybe_unify_known_subscript_type(const TypeHandle& source, types::Subscript& sub);
  TypeHandle maybe_unify_function_call_subscript(const TypeHandle& source, const types::Abstraction& source_func, types::Subscript& sub);

  void check_assignment(const TypeHandle& source, const types::Assignment& assignment);
  void check_push_func(const TypeHandle& source, const types::Abstraction& func);

  bool is_known_subscript_type(const TypeHandle& handle) const;
  bool is_concrete_argument(const TypeHandle& handle) const;
  bool are_concrete_arguments(const TypeHandles& handles) const;

  void flatten_list(const TypeHandle& source, std::vector<TypeHandle>& into) const;

  DebugTypePrinter type_printer() const;

private:
  TypeStore& store;
  const Library& library;
  StringRegistry& string_registry;
  Simplifier simplifier;

  std::vector<TypeEquation> type_equations;
  BoundVariables bound_variables;

  std::unordered_map<TypeHandle, bool, TypeHandle::Hash> registered_funcs;
  std::unordered_map<TypeHandle, bool, TypeHandle::Hash> registered_assignments;

  Instantiation instantiation;
};

}