#pragma once

#include "type.hpp"
#include <functional>
#include <cassert>

namespace mt {

class TypeStore;

class DestructuredMemberVisitor {
public:
  class Predicate {
  public:
    virtual bool predicate(TypeRef lhs, TypeRef rhs, bool rev) const = 0;
    virtual bool parameters(TypeRef lhs, TypeRef rhs, const types::Parameters& a,
      const types::DestructuredTuple& b, int64_t offset_b, bool rev) const = 0;
  };

private:
  using DT = types::DestructuredTuple;

public:
  explicit DestructuredMemberVisitor(const TypeStore& store, const Predicate& predicate) :
  store(store), predicate(predicate) {
    //
  }

public:
  bool expand_members(TypeRef lhs, TypeRef rhs, const DT& a, const DT& b, bool rev) const;

private:
  bool recurse_tuple(TypeRef lhs, TypeRef rhs, const DT& a, const DT& b, int64_t* ia, int64_t* ib, bool rev) const;
  bool subrecurse_tuple(TypeRef lhs, TypeRef rhs, const DT& a, const DT& b, int64_t* ib, int64_t expect_match, bool rev) const;
  bool subrecurse_list(const types::List& a, int64_t* ia, const DT& b, const TypeHandle& mem_b, bool rev) const;
  bool match_list(const types::List& a, const DT& b, int64_t* ib, bool rev) const;

  int64_t expect_to_match(const DT& parent, const DT& child) const;

private:
  const TypeStore& store;
  const Predicate& predicate;
};
}