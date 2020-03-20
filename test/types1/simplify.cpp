#include "simplify.hpp"
#include "unification.hpp"
#include "debug.hpp"
#include <cassert>

#define MT_SHOW2(msg, a, b) \
  std::cout << (msg) << std::endl; \
  unifier.type_printer().show2((a), (b)); \
  std::cout << std::endl

namespace mt {

Type::Tag Simplifier::type_of(const TypeHandle& handle) const {
  return store.at(handle).tag;
}

bool Simplifier::simplify_same_types(const TypeHandle& lhs, const TypeHandle& rhs) {
  using Tag = Type::Tag;

  const auto& t0 = unifier.store.at(lhs);
  const auto& t1 = unifier.store.at(rhs);

  switch (t0.tag) {
    case Tag::abstraction:
      return simplify(t0.abstraction, t1.abstraction);
    case Tag::scalar:
      return simplify(t0.scalar, t1.scalar);
    case Tag::tuple:
      return simplify(t0.tuple, t1.tuple);
    case Tag::destructured_tuple:
      return simplify(t0.destructured_tuple, t1.destructured_tuple);
    case Tag::list:
      return simplify(t0.list, t1.list);
    case Tag::variable: {
      unifier.push_type_equation(TypeEquation(lhs, rhs));
      return true;
    }
    default:
      MT_SHOW2("Unhandled same types for simplify: ", lhs, rhs);
      assert(false);
      return false;
  }
}

bool Simplifier::simplify_different_types(const TypeHandle& lhs, const TypeHandle& rhs) {
  using Tag = Type::Tag;

  const auto lhs_type = type_of(lhs);
  const auto rhs_type = type_of(rhs);

  if (lhs_type == Tag::variable) {
    push_type_equation(TypeEquation(lhs, rhs));
    return true;

  } else if (rhs_type == Tag::variable) {
    push_type_equation(TypeEquation(rhs, lhs));
    return true;

  } else if (lhs_type == Tag::destructured_tuple) {
    return simplify_different_types(store.at(lhs).destructured_tuple, lhs, rhs);

  } else if (rhs_type == Tag::destructured_tuple) {
    return simplify_different_types(store.at(rhs).destructured_tuple, rhs, lhs);

  } else if (lhs_type == Tag::list) {
    return simplify_different_types(store.at(lhs).list, lhs, rhs);

  } else if (rhs_type == Tag::list) {
    return simplify_different_types(store.at(rhs).list, rhs, lhs);

  } else {
    MT_SHOW2("Unhandled simplify for diff types: ", lhs, rhs);
    assert(false);
    return false;
  }
}

bool Simplifier::simplify(const TypeHandle& lhs, const TypeHandle& rhs) {
  if (type_of(lhs) == type_of(rhs)) {
    return simplify_same_types(lhs, rhs);
  } else {
    return simplify_different_types(lhs, rhs);
  }
}

bool Simplifier::simplify_different_types(const types::List& list, const TypeHandle& source, const TypeHandle& rhs) {
  const auto& rhs_member = store.at(rhs);
  if (rhs_member.is_scalar() || rhs_member.is_tuple()) {
    for (const auto& el : list.pattern) {
      push_type_equation(TypeEquation(el, rhs));
    }
    return true;
  }

  MT_SHOW2("Cannot simplify list with non-scalar type.", source, rhs);
  assert(false);

  return false;
}

bool Simplifier::simplify_different_types(const types::DestructuredTuple& tup, const TypeHandle& source, const TypeHandle& rhs) {
  using Use = types::DestructuredTuple::Usage;

  auto t = store.make_destructured_tuple(Use::rvalue, rhs);
  push_type_equation(TypeEquation(source, t));

  return true;
}

bool Simplifier::simplify(const types::Scalar& t0, const types::Scalar& t1) {
  return t0.identifier == t1.identifier;
}

bool Simplifier::simplify(const types::Abstraction& t0, const types::Abstraction& t1) {
  return simplify(t0.inputs, t1.inputs) && simplify(t0.outputs, t1.outputs);
}

bool Simplifier::simplify_expanding_members(const types::DestructuredTuple& a,
                                         const types::DestructuredTuple& b) {
  const int64_t num_a = a.members.size();
  const int64_t num_b = b.members.size();

  int64_t ia = 0;
  int64_t ib = 0;

  while (ia < num_a && ib < num_b) {
    bool res = simplify_recurse_tuple(a, b, &ia, &ib);
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

bool Simplifier::simplify_recurse_tuple(const types::DestructuredTuple& a,
                                     const types::DestructuredTuple& b,
                                     int64_t* ia, int64_t* ib) {
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
    return simplify_subrecurse_tuple(a, b, va.destructured_tuple, ia, ib);

  } else if (vb.is_destructured_tuple()) {
    return simplify_subrecurse_tuple(b, a, vb.destructured_tuple, ib, ia);

  } else {
    (*ia)++;
    (*ib)++;

    return simplify(mem_a, mem_b);
  }
}

bool Simplifier::simplify_subrecurse_tuple(const types::DestructuredTuple& a,
                                        const types::DestructuredTuple& b,
                                        const types::DestructuredTuple& sub_a,
                                        int64_t* ia, int64_t* ib) {
  int64_t ia_child = 0;
  const int64_t num_sub_a = sub_a.size();
  int64_t expect_match = num_sub_a;

  if (a.is_value_usage() && sub_a.is_outputs()) {
    expect_match = 1;
  }

  bool success = true;

  while (success && ia_child < expect_match && ia_child < num_sub_a && *ib < b.size()) {
    success = simplify_recurse_tuple(sub_a, b, &ia_child, ib);
  }

  (*ia)++;

  return success && ia_child == expect_match;
}

bool Simplifier::simplify_subrecurse_list(const types::List& a, int64_t* ia,
                                       const types::DestructuredTuple& b, const TypeHandle& mem_b) {

  const auto& mem_a = a.pattern[*ia];

  if (type_of(mem_b) == Type::Tag::destructured_tuple) {
    const auto& sub_b = store.at(mem_b).destructured_tuple;
    const int64_t num_b = sub_b.size();

    int64_t expect_num_b = num_b;
    if (b.is_value_usage() && sub_b.is_outputs()) {
      expect_num_b = 1;
    }

    int64_t ib = 0;
    bool success = true;

    while (success && ib < expect_num_b && ib < num_b) {
      success = simplify_subrecurse_list(a, ia, sub_b, sub_b.members[ib++]);
    }

    return success && ib == expect_num_b;

  } else {
    *ia = (*ia + 1) % a.size();
    return simplify(mem_a, mem_b);
  }
}

bool Simplifier::match_list(const types::List& a, const types::DestructuredTuple& b, int64_t* ib) {
  int64_t ia = 0;
  bool success = true;

  while (success && ia < a.size() && *ib < b.size()) {
    success = simplify_subrecurse_list(a, &ia, b, b.members[(*ib)++]);
  }

  return success && (a.size() == 0 || (ia == 0 && *ib == b.size()));
}

bool Simplifier::simplify(const types::DestructuredTuple& t0, const types::DestructuredTuple& t1) {
  if (types::DestructuredTuple::mismatching_definition_usages(t0, t1)) {
    return false;

  } else if (t0.is_definition_usage() && t0.usage == t1.usage) {
    return simplify(t0.members, t1.members);

  } else {
    return simplify_expanding_members(t0, t1);
  }
}

bool Simplifier::simplify(const types::List& t0, const types::List& t1) {
  const int64_t num_a = t0.size();
  const int64_t num_b = t1.size();
  const int64_t sz = std::max(num_a, num_b);

  if ((num_a == 0 || num_b == 0) && sz > 0) {
    //  Empty list with non-empty list.
    return false;
  }

  for (int64_t i = 0; i < sz; i++) {
    const auto& mem_a = t0.pattern[i % num_a];
    const auto& mem_b = t1.pattern[i % num_b];

    push_type_equation(TypeEquation(mem_a, mem_b));
  }

  return true;
}

bool Simplifier::simplify(const types::Tuple& t0, const types::Tuple& t1) {
  return simplify(t0.members, t1.members);
}

bool Simplifier::simplify(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1) {
  const auto sz0 = t0.size();
  if (sz0 != t1.size()) {
    return false;
  }
  push_type_equations(t0, t1, sz0);
  return true;
}

void Simplifier::push_type_equations(const std::vector<TypeHandle>& t0, const std::vector<TypeHandle>& t1, int64_t num) {
  assert(num >= 0 && num <= t0.size() && num <= t1.size() && "out of bounds read.");
  for (int64_t i = 0; i < num; i++) {
    push_type_equation(TypeEquation(t0[i], t1[i]));
  }
}

void Simplifier::push_type_equation(TypeEquation&& eq) {
  unifier.push_type_equation(std::move(eq));
}

}
