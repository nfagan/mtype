#pragma once

#include "type.hpp"

namespace mt {

class Unifier;
class TypeStore;

class Simplifier {
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

  bool simplify_expanding_members(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1);
  bool simplify_recurse_tuple(const types::DestructuredTuple& a, const types::DestructuredTuple& b, int64_t* ia, int64_t* ib);
  bool simplify_subrecurse_tuple(const types::DestructuredTuple& a, const types::DestructuredTuple& b,
                                 const types::DestructuredTuple& sub_a, int64_t* ia, int64_t* ib);

  bool match_list(const types::List& a, const types::DestructuredTuple& b, int64_t* ib);
  bool simplify_subrecurse_list(const types::List& a, int64_t* ia, const types::DestructuredTuple& b, const TypeHandle& mem_b);

  Type::Tag type_of(const TypeHandle& handle) const;
  void push_type_equations(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, int64_t num);
  void push_type_equation(TypeEquation&& eq);

private:
  Unifier& unifier;
  TypeStore& store;
};

}