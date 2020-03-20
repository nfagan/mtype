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
    bool operator()(const TypeHandle& a, const TypeHandle& b) {
      return simplifier.simplify(a, b);
    }

    Simplifier& simplifier;
  };
public:
  Simplifier(Unifier& unifier, TypeStore& store) : unifier(unifier), store(store) {
    //
  }

public:
  bool simplify(const TypeHandle& lhs, const TypeHandle& rhs);

private:
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

  void push_type_equations(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, int64_t num);
  void push_type_equation(TypeEquation&& eq);

private:
  Unifier& unifier;
  TypeStore& store;
};

}