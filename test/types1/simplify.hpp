#pragma once

#include "type.hpp"

namespace mt {

class Unifier;
class TypeStore;

class Simplifier {
private:
  struct DestructuredSimplifier {
    DestructuredSimplifier(Simplifier& simplifier) : simplifier(simplifier) {
      //
    }
    bool operator()(const TypeHandle& a, const TypeHandle& b, bool rev) {
      return simplifier.simplify(a, b, rev);
    }

    Simplifier& simplifier;
  };
public:
  Simplifier(Unifier& unifier, TypeStore& store) : unifier(unifier), store(store) {
    //
  }

public:
  bool simplify_entry(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs);

private:
  using DT = types::DestructuredTuple;

  bool simplify(TypeRef lhs, TypeRef rhs, bool rev);

  bool simplify_same_types(TypeRef lhs, TypeRef rhs, bool rev);
  bool simplify_different_types(TypeRef lhs, TypeRef rhs, bool rev);

  bool simplify(TypeRef lhs, TypeRef rhs, const types::Abstraction& t0, const types::Abstraction& t1, bool rev);
  bool simplify(TypeRef lhs, TypeRef rhs, const DT& t0, const DT& t1, bool rev);
  bool simplify(TypeRef lhs, TypeRef rhs, const types::Scalar& t0, const types::Scalar& t1, bool rev);
  bool simplify(const types::List& t0, const types::List& t1, bool rev);
  bool simplify(TypeRef lhs, TypeRef rhs, const types::Tuple& t0, const types::Tuple& t1, bool rev);
  bool simplify(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, bool rev);

  bool simplify_different_types(const types::List& list, TypeRef source, TypeRef rhs, bool rev);
  bool simplify_different_types(const types::DestructuredTuple& tup, TypeRef source, TypeRef rhs, bool rev);
  bool simplify_make_type_equation(TypeRef t0, TypeRef t1, bool rev);

  void push_make_type_equation(TypeRef t0, TypeRef t1, bool rev);
  void push_make_type_equations(const TypeHandles& t0, const TypeHandles& t1, int64_t num, bool rev);
  void push_type_equation(TypeEquation&& eq);

  void check_emplace_simplification_failure(bool success, TypeRef lhs, TypeRef rhs);

  const Token* lhs_source_token();
  const Token* rhs_source_token();

private:
  Unifier& unifier;
  TypeStore& store;

  TypeEquationTerm lhs_source_term;
  TypeEquationTerm rhs_source_term;
};

}