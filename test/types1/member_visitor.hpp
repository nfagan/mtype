#pragma once

#include "type.hpp"
#include <functional>
#include <cassert>

namespace mt {

class TypeStore;

class DestructuredMemberVisitor {
private:
  using DT = types::DestructuredTuple;
  using Pred = std::function<bool(const TypeHandle&, const TypeHandle&)>;

public:
  explicit DestructuredMemberVisitor(const TypeStore& store, const Pred& predicate) :
  store(store), predicate(predicate) {
    //
  }

public:
  bool expand_members(const DT& a, const DT& b) const;

private:
  bool recurse_tuple(const DT& a, const DT& b, int64_t* ia, int64_t* ib) const;
  bool subrecurse_tuple(const DT& a, const DT& b, int64_t* ib, int64_t expect_match) const;
  bool subrecurse_list(const types::List& a, int64_t* ia, const DT& b, const TypeHandle& mem_b) const;
  bool match_list(const types::List& a, const DT& b, int64_t* ib) const;

  int64_t expect_to_match(const DT& parent, const DT& child) const;

private:
  const TypeStore& store;
  const Pred& predicate;
};
}