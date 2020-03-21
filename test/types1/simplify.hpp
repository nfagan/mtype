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

  bool simplify(const TypeHandle& lhs, const TypeHandle& rhs, bool rev);

  bool simplify_same_types(const TypeHandle& lhs, const TypeHandle& rhs, bool rev);
  bool simplify_different_types(const TypeHandle& lhs, const TypeHandle& rhs, bool rev);

  bool simplify(const types::Abstraction& t0, const types::Abstraction& t1, bool rev);
  bool simplify(const types::Scalar& t0, const types::Scalar& t1, bool rev);
  bool simplify(const types::Tuple& t0, const types::Tuple& t1, bool rev);
  bool simplify(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1, bool rev);
  bool simplify(const types::List& t0, const types::List& t1, bool rev);
  bool simplify(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, bool rev);

  bool simplify_different_types(const types::List& list, const TypeHandle& source, const TypeHandle& rhs, bool rev);
  bool simplify_different_types(const types::DestructuredTuple& tup, const TypeHandle& source, const TypeHandle& rhs, bool rev);
  bool simplify_make_type_equation(const TypeHandle& t0, const TypeHandle& t1, bool rev);

  void push_make_type_equation(const TypeHandle& t0, const TypeHandle& t1, bool rev);
  void push_type_equations(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, int64_t num, bool rev);
  void push_type_equation(TypeEquation&& eq);

  const Token* lhs_source_token();
  const Token* rhs_source_token();

private:
  Unifier& unifier;
  TypeStore& store;

  TypeEquationTerm lhs_source_term;
  TypeEquationTerm rhs_source_term;
};

}