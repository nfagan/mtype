#include "union_member_visitor.hpp"
#include "types.hpp"
#include <unordered_set>
#include <cassert>

namespace mt {

UnionMemberVisitor::UnionMemberVisitor(const Predicate* predicate) :
predicate(predicate) {
  //
}

bool UnionMemberVisitor::apply_element_wise(const TypePtrs& a,
                                            const TypePtrs& b,
                                            int64_t sz,
                                            bool rev) const {
  for (int64_t i = 0; i < sz; i++) {
    if (!(*predicate)(a[i], b[i], rev)) {
      return false;
    }
  }
  return true;
}

bool UnionMemberVisitor::operator()(const types::Union& a, const types::Union& b, bool rev) const {
  const auto unique_a = types::Union::unique_members(a);
  const auto unique_b = types::Union::unique_members(b);

  const auto num_a = unique_a.count();
  const auto num_b = unique_b.count();

  if (num_a == num_b) {
    return apply_element_wise(unique_a.members, unique_b.members, num_a, rev);
  }

  const TypePtrs* expect_smaller_set = &unique_a.members;
  const TypePtrs* expect_larger_set = &unique_b.members;
  int64_t expect_smaller_num = num_a;
  int64_t expect_larger_num = num_b;

  if (rev) {
    std::swap(expect_smaller_set, expect_larger_set);
    std::swap(expect_smaller_num, expect_larger_num);
    rev = !rev;
  }

  if (expect_smaller_num > expect_larger_num) {
    return false;
  } else {
    return apply_element_wise(*expect_smaller_set, *expect_larger_set,
                              expect_smaller_num, rev);
  }
}

}
