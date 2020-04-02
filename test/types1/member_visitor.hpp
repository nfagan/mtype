#pragma once

#include "type.hpp"
#include <functional>
#include <cassert>

namespace mt {

class TypeStore;

template<typename T>
class DestructuredMemberVisitor {
public:
  class Predicate {
  public:
    virtual bool predicate(T lhs, T rhs, bool rev) const = 0;
    virtual bool parameters(T lhs, T rhs, const types::Parameters& a,
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
  bool expand_members(T lhs, T rhs, const DT& a, const DT& b, bool rev) const;

private:
  bool recurse_tuple(T lhs, T rhs, const DT& a, const DT& b, int64_t* ia, int64_t* ib, bool rev) const;
  bool subrecurse_tuple(T lhs, T rhs, const DT& a, const DT& b, int64_t* ib, int64_t expect_match, bool rev) const;
  bool subrecurse_list(const types::List& a, int64_t* ia, const DT& b, T mem_b, bool rev) const;
  bool match_list(const types::List& a, const DT& b, int64_t* ib, bool rev) const;

  int64_t expect_to_match(const DT& parent, const DT& child) const;

private:
  const TypeStore& store;
  const Predicate& predicate;
};


template<typename T>
int64_t DestructuredMemberVisitor<T>::expect_to_match(const DT& parent, const DT& child) const {
  if (parent.is_value_usage() && child.is_outputs()) {
    return 1;
  } else {
    return child.size();
  }
}

template<typename T>
bool DestructuredMemberVisitor<T>::expand_members(T lhs, T rhs, const DT& a, const DT& b, bool rev) const {
  const int64_t num_a = a.size();
  const int64_t num_b = b.size();

  int64_t ia = 0;
  int64_t ib = 0;

  while (ia < num_a && ib < num_b) {
    bool res = recurse_tuple(lhs, rhs, a, b, &ia, &ib, rev);
    if (!res) {
      return false;
    }
  }

  if (ia == num_a && ib == num_b) {
    return true;

  } else if (a.is_outputs() && b.is_value_usage()) {
    return ib == num_b && ia == num_b;

  } else if (b.is_outputs() && a.is_value_usage()) {
    return ia == num_a && ib == num_a;

  } else {
    return false;
  }
}

template<typename T>
bool DestructuredMemberVisitor<T>::recurse_tuple(T lhs, T rhs, const DT& a, const DT& b,
                                                 int64_t* ia, int64_t* ib, bool rev) const {
  assert(*ia < a.size() && *ib < b.size());

  const auto mem_a = a.members[*ia];
  const auto mem_b = b.members[*ib];

  if (mem_a->is_parameters()) {
    bool success = predicate.parameters(mem_a, rhs, MT_PARAMS_REF(*mem_a), b, *ib, rev);
    (*ia)++;
    *ib = b.size();
    return success;

  } else if (mem_b->is_parameters()) {
    bool success = predicate.parameters(mem_b, lhs, MT_PARAMS_REF(*mem_b), a, *ia, !rev);
    (*ib)++;
    *ia = a.size();
    return success;

  } else if (mem_a->is_list() && !mem_b->is_list()) {
    if (b.is_definition_usage() && !mem_b->is_variable()) {
      return false;
    }

    bool success = match_list(MT_LIST_REF(*mem_a), b, ib, rev);
    (*ia)++;
    return success;

  } else if (mem_b->is_list() && !mem_a->is_list()) {
    if (a.is_definition_usage() && !mem_a->is_variable()) {
      return false;
    }

    bool success = match_list(MT_LIST_REF(*mem_b), a, ia, !rev);
    (*ib)++;
    return success;

  } else if (mem_a->is_destructured_tuple()) {
    const int64_t expect_match = expect_to_match(a, MT_DT_REF(*mem_a));
    bool success = subrecurse_tuple(mem_a, rhs, MT_DT_REF(*mem_a), b, ib, expect_match, rev);
    (*ia)++;
    return success;

  } else if (mem_b->is_destructured_tuple()) {
    const int64_t expect_match = expect_to_match(b, MT_DT_REF(*mem_b));
    bool success = subrecurse_tuple(mem_b, lhs, MT_DT_REF(*mem_b), a, ia, expect_match, !rev);
    (*ib)++;
    return success;

  } else {
    (*ia)++;
    (*ib)++;

    return predicate.predicate(mem_a, mem_b, rev);
  }
}

template<typename T>
bool DestructuredMemberVisitor<T>::subrecurse_tuple(T lhs, T rhs, const DT& child_a, const DT& b,
                                                    int64_t* ib, int64_t expect_match, bool rev) const {
  int64_t ia_child = 0;
  bool success = true;

  while (success && ia_child < expect_match && ia_child < child_a.size() && *ib < b.size()) {
    success = recurse_tuple(lhs, rhs, child_a, b, &ia_child, ib, rev);
  }

  return success && ia_child == expect_match;
}

template<typename T>
bool DestructuredMemberVisitor<T>::subrecurse_list(const types::List& a, int64_t* ia,
                                                   const DT& b, T mem_b, bool rev) const {

  const auto& mem_a = a.pattern[*ia];

  if (mem_b->is_destructured_tuple()) {
    const auto& sub_b = MT_DT_REF(*mem_b);
    const int64_t expect_num_b = expect_to_match(b, sub_b);
    int64_t ib = 0;
    bool success = true;

    while (success && ib < expect_num_b && ib < sub_b.size()) {
      success = subrecurse_list(a, ia, sub_b, sub_b.members[ib++], rev);
    }

    return success && ib == expect_num_b;

  } else {
    *ia = (*ia + 1) % a.size();
    return predicate.predicate(mem_a, mem_b, rev);
  }
}

template<typename T>
bool DestructuredMemberVisitor<T>::match_list(const types::List& a, const DT& b, int64_t* ib, bool rev) const {
  int64_t ia = 0;
  bool success = true;

  while (success && ia < a.size() && *ib < b.size()) {
    success = subrecurse_list(a, &ia, b, b.members[(*ib)++], rev);
  }

  return success && (a.size() == 0 || (ia == 0 && *ib == b.size()));
}





}