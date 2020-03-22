#include "simplify.hpp"
#include "unification.hpp"
#include "member_visitor.hpp"
#include "debug.hpp"
#include <cassert>

#define MT_SHOW2(msg, a, b) \
  std::cout << (msg) << std::endl; \
  unifier.type_printer().show2((a), (b)); \
  std::cout << std::endl

namespace mt {

bool Simplifier::simplify_entry(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs) {
  lhs_source_term = lhs;
  rhs_source_term = rhs;

  return simplify(lhs.term, rhs.term, false);
}

bool Simplifier::simplify(TypeRef lhs, TypeRef rhs, bool rev) {
  if (store.type_of(lhs) == store.type_of(rhs)) {
    return simplify_same_types(lhs, rhs, rev);
  } else {
    return simplify_different_types(lhs, rhs, rev);
  }
}

bool Simplifier::simplify_same_types(TypeRef lhs, TypeRef rhs, bool rev) {
  using Tag = Type::Tag;

  const auto& t0 = store.at(lhs);
  const auto& t1 = store.at(rhs);

  switch (t0.tag) {
    case Tag::abstraction:
      return simplify(lhs, rhs, t0.abstraction, t1.abstraction, rev);
    case Tag::scalar:
      return simplify(lhs, rhs, t0.scalar, t1.scalar, rev);
    case Tag::tuple:
      return simplify(lhs, rhs, t0.tuple, t1.tuple, rev);
    case Tag::destructured_tuple:
      return simplify(lhs, rhs, t0.destructured_tuple, t1.destructured_tuple, rev);
    case Tag::list:
      return simplify(t0.list, t1.list, rev);
    case Tag::variable:
      return simplify_make_type_equation(lhs, rhs, rev);
    default:
      MT_SHOW2("Unhandled same types for simplify: ", lhs, rhs);
      assert(false);
      return false;
  }
}

bool Simplifier::simplify_make_type_equation(TypeRef t0, TypeRef t1, bool rev) {
  push_make_type_equation(t0, t1, rev);
  return true;
}

bool Simplifier::simplify_different_types(TypeRef lhs, TypeRef rhs, bool rev) {
  using Tag = Type::Tag;

  const auto lhs_type = store.type_of(lhs);
  const auto rhs_type = store.type_of(rhs);

  if (lhs_type == Type::Tag::variable || rhs_type == Type::Tag::variable) {
    return simplify_make_type_equation(lhs, rhs, rev);
  }

  if (lhs_type == Tag::destructured_tuple) {
    return simplify_different_types(store.at(lhs).destructured_tuple, lhs, rhs, rev);

  } else if (rhs_type == Tag::destructured_tuple) {
    return simplify_different_types(store.at(rhs).destructured_tuple, rhs, lhs, !rev);

  } else if (lhs_type == Tag::list) {
    return simplify_different_types(store.at(lhs).list, lhs, rhs, rev);

  } else if (rhs_type == Tag::list) {
    return simplify_different_types(store.at(rhs).list, rhs, lhs, !rev);

  } else {
    check_emplace_simplification_failure(false, lhs, rhs);
//    MT_SHOW2("Unhandled simplify for diff types: ", lhs, rhs);
//    assert(false);
    return false;
  }
}

bool Simplifier::simplify_different_types(const types::List& list, TypeRef source,
                                          TypeRef rhs, bool rev) {
  const auto& rhs_member = store.at(rhs);

  if (rhs_member.is_scalar() || rhs_member.is_tuple()) {
    for (const auto& el : list.pattern) {
      push_make_type_equation(el, rhs, rev);
    }

    return true;
  }

  MT_SHOW2("ERROR: Cannot simplify list with type:", source, rhs);
  assert(false);

  return false;
}

bool Simplifier::simplify_different_types(const DT& tup, TypeRef source, TypeRef rhs, bool rev) {
  const auto t = store.make_rvalue_destructured_tuple(rhs);
  return simplify_make_type_equation(source, t, rev);
}

bool Simplifier::simplify(TypeRef lhs, TypeRef rhs, const types::Scalar& t0, const types::Scalar& t1, bool rev) {
  bool success = t0.identifier == t1.identifier;
  check_emplace_simplification_failure(success, lhs, rhs);
  return success;
}

bool Simplifier::simplify(TypeRef lhs, TypeRef rhs, const types::Abstraction& t0, const types::Abstraction& t1, bool rev) {
  //  Contravariance for inputs.
  bool success = simplify(t0.inputs, t1.inputs, !rev);
  success = success && simplify(t0.outputs, t1.outputs, rev);
  check_emplace_simplification_failure(success, lhs, rhs);
  return success;
}

bool Simplifier::simplify(TypeRef lhs, TypeRef rhs, const DT& t0, const DT& t1, bool rev) {
  bool success;

  if (types::DestructuredTuple::mismatching_definition_usages(t0, t1)) {
    success = false;

  } else if (t0.is_definition_usage() && t0.usage == t1.usage) {
    success = simplify(t0.members, t1.members, rev);

  } else {
    success = DestructuredMemberVisitor{store, DestructuredSimplifier{*this}}.expand_members(t0, t1, rev);
  }

  check_emplace_simplification_failure(success, lhs, rhs);
  return success;
}

bool Simplifier::simplify(const types::List& t0, const types::List& t1, bool rev) {
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

    push_make_type_equation(mem_a, mem_b, rev);
  }

  return true;
}

bool Simplifier::simplify(TypeRef lhs, TypeRef rhs, const types::Tuple& t0, const types::Tuple& t1, bool rev) {
  bool success = simplify(t0.members, t1.members, rev);
  check_emplace_simplification_failure(success, lhs, rhs);
  return success;
}

bool Simplifier::simplify(const TypeHandles& t0, const TypeHandles& t1, bool rev) {
  const auto sz0 = t0.size();
  if (sz0 != t1.size()) {
    return false;
  }
  push_make_type_equations(t0, t1, sz0, rev);
  return true;
}

void Simplifier::push_make_type_equations(const TypeHandles& t0, const TypeHandles& t1, int64_t num, bool rev) {
  assert(num >= 0 && num <= t0.size() && num <= t1.size() && "out of bounds read.");
  for (int64_t i = 0; i < num; i++) {
    push_make_type_equation(t0[i], t1[i], rev);
  }
}

void Simplifier::push_type_equation(TypeEquation&& eq) {
  unifier.push_type_equation(std::move(eq));
}

void Simplifier::push_make_type_equation(TypeRef t0, TypeRef t1, bool rev) {
  if (rev) {
    const auto lhs_term = make_term(rhs_source_token(), t1);
    const auto rhs_term = make_term(lhs_source_token(), t0);
    push_type_equation(make_eq(lhs_term, rhs_term));

  } else {
    const auto lhs_term = make_term(lhs_source_token(), t0);
    const auto rhs_term = make_term(rhs_source_token(), t1);

    push_type_equation(make_eq(lhs_term, rhs_term));
  }
}

void Simplifier::check_emplace_simplification_failure(bool success, TypeRef lhs, TypeRef rhs) {
  if (!success) {
    unifier.emplace_simplification_failure(lhs_source_token(), rhs_source_token(), lhs, rhs);
  }
}

const Token* Simplifier::lhs_source_token() {
  return lhs_source_term.source_token;
}

const Token* Simplifier::rhs_source_token() {
  return rhs_source_term.source_token;
}

}
