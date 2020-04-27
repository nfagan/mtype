#pragma once

#include "type.hpp"

namespace mt {

namespace types {
  struct Union;
}

class UnionMemberVisitor {
public:
  class Predicate {
  public:
    virtual bool operator()(Type* lhs, Type* rhs, bool rev) const = 0;
  };

public:
  UnionMemberVisitor(const Predicate* predicate);
  bool operator()(const types::Union& a, const types::Union& b, bool rev) const;

private:
  bool apply_element_wise(const TypePtrs& a, const TypePtrs& b,
                          int64_t sz, bool rev) const;
  bool check_subsumption(const TypePtrs& a, const TypePtrs& b,
                         int64_t num_a, int64_t num_b, bool rev) const;

private:
  const Predicate* predicate;
};
}