#include "member_visitor.hpp"
#include "type_store.hpp"

namespace mt {

int64_t DestructuredMemberVisitor::expect_to_match(const types::DestructuredTuple& parent, const types::DestructuredTuple& child) const {
  if (parent.is_value_usage() && child.is_outputs()) {
    return 1;
  } else {
    return child.size();
  }
}

bool DestructuredMemberVisitor::expand_members(const DT& a, const DT& b) const {
  const int64_t num_a = a.size();
  const int64_t num_b = b.size();

  int64_t ia = 0;
  int64_t ib = 0;

  while (ia < num_a && ib < num_b) {
    bool res = recurse_tuple(a, b, &ia, &ib);
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

bool DestructuredMemberVisitor::recurse_tuple(const DT& a, const DT& b, int64_t* ia, int64_t* ib) const {
  assert(*ia < a.size() && *ib < b.size());

  const auto& mem_a = a.members[*ia];
  const auto& mem_b = b.members[*ib];

  const auto& va = store.at(mem_a);
  const auto& vb = store.at(mem_b);

  if (va.is_list() && !vb.is_list()) {
    if (b.is_definition_usage()) {
      return false;
    }

    bool success = match_list(va.list, b, ib);
    (*ia)++;
    return success;

  } else if (vb.is_list() && !va.is_list()) {
    if (a.is_definition_usage()) {
      return false;
    }

    bool success = match_list(vb.list, a, ia);
    (*ib)++;
    return success;

  } else if (va.is_destructured_tuple()) {
    const int64_t expect_match = expect_to_match(a, va.destructured_tuple);
    bool success = subrecurse_tuple(va.destructured_tuple, b, ib, expect_match);
    (*ia)++;
    return success;


  } else if (vb.is_destructured_tuple()) {
    const int64_t expect_match = expect_to_match(b, vb.destructured_tuple);
    bool success = subrecurse_tuple(vb.destructured_tuple, a, ia, expect_match);
    (*ib)++;
    return success;

  } else {
    (*ia)++;
    (*ib)++;

    return predicate(mem_a, mem_b);
  }
}

bool DestructuredMemberVisitor::subrecurse_tuple(const DT& child_a, const DT& b, int64_t* ib, int64_t expect_match) const {
  int64_t ia_child = 0;
  bool success = true;

  while (success && ia_child < expect_match && ia_child < child_a.size() && *ib < b.size()) {
    success = recurse_tuple(child_a, b, &ia_child, ib);
  }

  return success && ia_child == expect_match;
}

bool DestructuredMemberVisitor::subrecurse_list(const types::List& a, int64_t* ia,
                                                const DT& b, const TypeHandle& mem_b) const {

  const auto& mem_a = a.pattern[*ia];
  const auto& vb = store.at(mem_b);

  if (vb.is_destructured_tuple()) {
    const auto& sub_b = vb.destructured_tuple;
    const int64_t expect_num_b = expect_to_match(b, sub_b);
    int64_t ib = 0;
    bool success = true;

    while (success && ib < expect_num_b && ib < sub_b.size()) {
      success = subrecurse_list(a, ia, sub_b, sub_b.members[ib++]);
    }

    return success && ib == expect_num_b;

  } else {
    *ia = (*ia + 1) % a.size();
    return predicate(mem_a, mem_b);
  }
}

bool DestructuredMemberVisitor::match_list(const types::List& a, const DT& b, int64_t* ib) const {
  int64_t ia = 0;
  bool success = true;

  while (success && ia < a.size() && *ib < b.size()) {
    success = subrecurse_list(a, &ia, b, b.members[(*ib)++]);
  }

  return success && (a.size() == 0 || (ia == 0 && *ib == b.size()));
}


}