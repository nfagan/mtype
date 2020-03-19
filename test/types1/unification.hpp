#pragma once

#include "mt/mt.hpp"
#include "type.hpp"
#include "type_equality.hpp"
#include "type_store.hpp"
#include "instance.hpp"
#include <map>

namespace mt {

class TypeVisitor;
class DebugTypePrinter;

struct TypeEquationTerm {
  TypeEquationTerm(const Token& source_token, const TypeHandle& type) :
  source_token(source_token), type(type) {
    //
  }


  Token source_token;
  TypeHandle type;
};

struct TypeEquation {
  TypeEquation(const TypeHandle& lhs, const TypeHandle& rhs) : lhs(lhs), rhs(rhs) {
    //
  }

  TypeHandle lhs;
  TypeHandle rhs;
};

class Unifier {
public:
  using BoundVariables = std::unordered_map<TypeHandle, TypeHandle, TypeHandle::Hash>;
public:
  explicit Unifier(TypeStore& store, StringRegistry& string_registry) :
  store(store),
  string_registry(string_registry),
  type_eq(store, string_registry),
  arg_comparator(type_eq),
  type_equiv_comparator(type_eq),
  function_types(arg_comparator),
  types_with_known_subscripts(type_equiv_comparator),
  instantiation(store) {
    //
  }

  ~Unifier() {
    std::cout << "Num type eqs: " << type_equations.size() << std::endl;
    std::cout << "Subs size: " << bound_variables.size() << std::endl;
  }

  void push_type_equation(TypeEquation&& eq) {
    type_equations.emplace_back(std::move(eq));
  }

  void unify();
  Optional<TypeHandle> bound_type(const TypeHandle& for_type) const;

private:
  void unify_one(TypeEquation eq);

  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, types::Abstraction& func);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, types::Variable& var);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, types::Tuple& tup);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, types::DestructuredTuple& tup);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, types::Subscript& sub);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source, types::List& list);
  [[nodiscard]] TypeHandle apply_to(const TypeHandle& source);
  void apply_to(std::vector<TypeHandle>& sources);

  [[nodiscard]] TypeHandle substitute_one(types::Variable& var, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::Abstraction& func, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::Tuple& tup, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::DestructuredTuple& tup, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::Subscript& sub, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs);
  [[nodiscard]] TypeHandle substitute_one(types::List& list, const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs);
  [[nodiscard]] TypeHandle substitute_one(const TypeHandle& source, const TypeHandle& lhs, const TypeHandle& rhs);
  void substitute_one(std::vector<TypeHandle>& sources, const TypeHandle& lhs, const TypeHandle& rhs);

  bool simplify(const TypeHandle& lhs, const TypeHandle& rhs);
  bool simplify_same_types(const TypeHandle& lhs, const TypeHandle& rhs);
  bool simplify_different_types(const TypeHandle& lhs, const TypeHandle& rhs);

  bool simplify(const types::Abstraction& t0, const types::Abstraction& t1);
  bool simplify(const types::Scalar& t0, const types::Scalar& t1);
  bool simplify(const types::Tuple& t0, const types::Tuple& t1);
  bool simplify(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1);
  bool simplify(const types::List& t0, const types::List& t1);
  bool simplify(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1);

  bool simplify_different_types(const types::List& list, const TypeHandle& source, const TypeHandle& rhs);
  bool simplify_different_types(const types::DestructuredTuple& tup, const TypeHandle& source, const TypeHandle& rhs);

  bool simplify_expanding_members(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1);
  bool simplify_recurse_tuple(const types::DestructuredTuple& a, const types::DestructuredTuple& b, int64_t* ia, int64_t* ib);
  bool simplify_subrecurse_tuple(const types::DestructuredTuple& a, const types::DestructuredTuple& b,
                                 const types::DestructuredTuple& sub_a, int64_t* ia, int64_t* ib);

  bool match_list(const types::List& a, const types::DestructuredTuple& b, int64_t* ib);
  bool simplify_subrecurse_list(const types::List& a, int64_t* ia, const types::DestructuredTuple& b, const TypeHandle& mem_b);

  Type::Tag type_of(const TypeHandle& handle) const;
  void show();

  void maybe_unify_subscript(const TypeHandle& source, types::Subscript& sub);
  bool maybe_unify_known_subscript_type(const TypeHandle& source, types::Subscript& sub);

  void check_push_func(const TypeHandle& source, const types::Abstraction& func);
  void push_type_equations(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, int64_t num);

  bool is_concrete_argument(const types::DestructuredTuple& tup) const;
  bool is_concrete_argument(const types::List& list) const;
  bool is_concrete_argument(const TypeHandle& arg) const;
  bool are_concrete_arguments(const std::vector<TypeHandle>& args) const;

  bool is_known_subscript_type(const TypeHandle& handle) const;

  void flatten_destructured_tuple(const types::DestructuredTuple& source, std::vector<TypeHandle>& into) const;
  void flatten_list(const TypeHandle& source, std::vector<TypeHandle>& into) const;

  void make_known_types();
  void make_binary_operators();
  void make_subscript_references();
  void make_builtin_parens_subscript_references();
  void make_builtin_brace_subscript_reference();
  void make_free_functions();
  void make_min();
  void make_fileparts();
  void make_list_outputs_type();
  void make_list_outputs_type2();
  void make_list_inputs_type();

  DebugTypePrinter type_printer() const;

private:
  TypeStore& store;
  StringRegistry& string_registry;
  TypeEquality type_eq;

  std::vector<TypeEquation> type_equations;
  BoundVariables bound_variables;

  TypeEquality::ArgumentComparator arg_comparator;
  TypeEquality::TypeEquivalenceComparator type_equiv_comparator;
  std::unordered_map<TypeHandle, bool, TypeHandle::Hash> registered_funcs;
  std::map<types::Abstraction, TypeHandle, TypeEquality::ArgumentComparator> function_types;
  std::set<TypeHandle, TypeEquality::TypeEquivalenceComparator> types_with_known_subscripts;

  Instantiation instantiation;
};

}