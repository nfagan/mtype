#pragma once

#include "types.hpp"

namespace mt {

class Unifier;
class TypeStore;
struct Token;

class Simplifier {
  friend class DestructuredSimplifier;
public:
  Simplifier(Unifier& unifier, TypeStore& store) : unifier(unifier), store(store) {
    //
  }

public:
  bool simplify_entry(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs);

private:
  using DT = types::DestructuredTuple;

  bool simplify(Type* lhs, Type* rhs, bool rev);

  bool simplify_same_types(Type* lhs, Type* rhs, bool rev);
  bool simplify_different_types(Type* lhs, Type* rhs, bool rev);

  bool simplify(Type* lhs, Type* rhs, const DT& t0, const DT& t1, bool rev);
  bool simplify(const types::Abstraction& t0, const types::Abstraction& t1, bool rev);
  bool simplify(const types::Scalar& t0, const types::Scalar& t1, bool rev);
  bool simplify(const types::Scheme& t0, const types::Scheme& t1, bool rev);
  bool simplify(const types::Tuple& t0, const types::Tuple& t1, bool rev);
  bool simplify(const types::Subscript& t0, const types::Subscript& t1, bool rev);
  bool simplify(const types::List& t0, const types::List& t1, bool rev);
  bool simplify(const types::Class& t0, const types::Class& t1, bool rev);
  bool simplify(const types::Record& t0, const types::Record& t1, bool rev);
  bool simplify(const types::ConstantValue& t0, const types::ConstantValue& t1, bool rev);
  bool simplify(const TypePtrs& t0, const TypePtrs& t1, bool rev);

  bool simplify_different_types(const types::Scheme& scheme, Type* source, Type* rhs, bool rev);
  bool simplify_different_types(const types::List& list, Type* rhs, bool rev);
  bool simplify_different_types(const types::DestructuredTuple& tup, Type* source, Type* rhs, bool rev);
  bool simplify_different_types(Type* lhs, Type* rhs, const types::Parameters& a,
                                const types::DestructuredTuple& b, int64_t offset_b, bool rev);
  bool simplify_make_type_equation(Type* t0, Type* t1, bool rev);

  void push_make_type_equation(Type* t0, Type* t1, bool rev);
  void push_make_type_equations(const TypePtrs& t0, const TypePtrs& t1, int64_t num, bool rev);
  void push_type_equation(const TypeEquation& eq);

  void check_emplace_simplification_failure(bool success, const Type* lhs, const Type* rhs);

  const Token* lhs_source_token();
  const Token* rhs_source_token();

private:
  Unifier& unifier;
  TypeStore& store;

  TypeEquationTerm lhs_source_term;
  TypeEquationTerm rhs_source_term;
};

}