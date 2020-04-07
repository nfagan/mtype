#include "simplify.hpp"
#include "unification.hpp"
#include "member_visitor.hpp"
#include "debug.hpp"
#include "../token.hpp"
#include <cassert>

#define MT_SHOW2(msg, a, b) \
  std::cout << (msg) << std::endl; \
  unifier.type_printer().show2((a), (b)); \
  std::cout << std::endl

namespace mt {

class DestructuredSimplifier : public DestructuredMemberVisitor<Type*>::Predicate {
public:
  explicit DestructuredSimplifier(Simplifier& simplifier) : simplifier(simplifier) {
    //
  }

  bool predicate(Type* a, Type* b, bool rev) const override {
    return simplifier.simplify(a, b, rev);
  }

  bool parameters(Type* lhs, Type* rhs, const types::Parameters& a,
                  const types::DestructuredTuple& b, int64_t offset_b, bool rev) const override {
    return simplifier.simplify_different_types(lhs, rhs, a, b, offset_b, rev);
  }

  Simplifier& simplifier;
};

bool Simplifier::simplify_entry(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs) {
  lhs_source_term = lhs;
  rhs_source_term = rhs;

  return simplify(lhs.term, rhs.term, false);
}

bool Simplifier::simplify(Type* lhs, Type* rhs, bool rev) {
  if (lhs->tag == rhs->tag) {
    return simplify_same_types(lhs, rhs, rev);
  } else {
    return simplify_different_types(lhs, rhs, rev);
  }
}

bool Simplifier::simplify_same_types(Type* lhs, Type* rhs, bool rev) {
  using Tag = Type::Tag;

  switch (lhs->tag) {
    case Tag::abstraction:
      return simplify(lhs, rhs, MT_ABSTR_REF(*lhs), MT_ABSTR_REF(*rhs), rev);
    case Tag::scalar:
      return simplify(lhs, rhs, MT_SCALAR_REF(*lhs), MT_SCALAR_REF(*rhs), rev);
    case Tag::tuple:
      return simplify(lhs, rhs, MT_TUPLE_REF(*lhs), MT_TUPLE_REF(*rhs), rev);
    case Tag::destructured_tuple:
      return simplify(lhs, rhs, MT_DT_REF(*lhs), MT_DT_REF(*rhs), rev);
    case Tag::list:
      return simplify(MT_LIST_REF(*lhs), MT_LIST_REF(*rhs), rev);
    case Tag::scheme:
      return simplify(lhs, rhs, MT_SCHEME_REF(*lhs), MT_SCHEME_REF(*rhs), rev);
    case Tag::subscript:
      return simplify(MT_SUBS_REF(*lhs), MT_SUBS_REF(*rhs), rev);
    case Tag::class_type:
      return simplify(MT_CLASS_REF(*lhs), MT_CLASS_REF(*rhs), rev);
    case Tag::variable:
      return simplify_make_type_equation(lhs, rhs, rev);
    default:
      MT_SHOW2("Unhandled same types for simplify: ", lhs, rhs);
      assert(false);
      return false;
  }
}

bool Simplifier::simplify_make_type_equation(Type* t0, Type* t1, bool rev) {
  push_make_type_equation(t0, t1, rev);
  return true;
}

bool Simplifier::simplify_different_types(Type* lhs, Type* rhs, bool rev) {
  if (lhs->is_variable() || rhs->is_variable()) {
    return simplify_make_type_equation(lhs, rhs, rev);
  }

  if (lhs->is_destructured_tuple()) {
    return simplify_different_types(MT_DT_REF(*lhs), lhs, rhs, rev);

  } else if (rhs->is_destructured_tuple()) {
    return simplify_different_types(MT_DT_REF(*rhs), rhs, lhs, !rev);

  } else if (lhs->is_list()) {
    return simplify_different_types(MT_LIST_REF(*lhs), rhs, rev);

  } else if (rhs->is_list()) {
    return simplify_different_types(MT_LIST_REF(*rhs), lhs, !rev);

  } else if (lhs->is_scheme()) {
    return simplify_different_types(MT_SCHEME_REF(*lhs), lhs, rhs, rev);

  } else if (rhs->is_scheme()) {
    return simplify_different_types(MT_SCHEME_REF(*rhs), rhs, lhs, !rev);

  } else {
    check_emplace_simplification_failure(false, lhs, rhs);
    return false;
  }
}

bool Simplifier::simplify_different_types(const types::Scheme& scheme, Type*, Type* rhs, bool rev) {
#if 1
  return simplify_make_type_equation(unifier.instantiate(scheme), rhs, rev);
#else
  check_emplace_simplification_failure(false, source, rhs);
  return false;
#endif
}

bool Simplifier::simplify_different_types(const types::List& list, Type* rhs, bool rev) {
  if (rhs->is_scalar() || rhs->is_tuple() || rhs->is_abstraction()) {
    for (const auto& el : list.pattern) {
      push_make_type_equation(el, rhs, rev);
    }

    return true;
  }

  check_emplace_simplification_failure(false, &list, rhs);
  return false;
}

bool Simplifier::simplify_different_types(const DT&, Type* source, Type* rhs, bool rev) {
  const auto t = store.make_rvalue_destructured_tuple(rhs);
  return simplify_make_type_equation(source, t, rev);
}

bool Simplifier::simplify_different_types(Type* lhs, Type*, const types::Parameters&,
                                          const types::DestructuredTuple& b, int64_t offset_b, bool) {
  if (unifier.expanded_parameters.count(lhs) > 0) {
    assert(false);
    return true;
  }

  TypePtrs match_members;
  for (int64_t i = offset_b; i < b.size(); i++) {
    match_members.push_back(b.members[i]);
  }

  const auto lhs_tup = store.make_rvalue_destructured_tuple(std::move(match_members));
  unifier.expanded_parameters[lhs] = lhs_tup;
  return true;
}

bool Simplifier::simplify(Type* lhs, Type* rhs, const types::Scalar& t0, const types::Scalar& t1, bool rev) {
  const bool success = unifier.subtype_relationship.related(lhs, rhs, t0, t1, rev);
  check_emplace_simplification_failure(success, lhs, rhs);
  return success;
}

bool Simplifier::simplify(Type* lhs, Type* rhs, const types::Abstraction& t0, const types::Abstraction& t1, bool rev) {
  //  Contravariance for inputs.
  bool success = simplify(t0.inputs, t1.inputs, !rev);
  success = success && simplify(t0.outputs, t1.outputs, rev);
  check_emplace_simplification_failure(success, lhs, rhs);
  return success;
}

bool Simplifier::simplify(Type* lhs, Type* rhs, const DT& t0, const DT& t1, bool rev) {
  bool success;

  if (types::DestructuredTuple::mismatching_definition_usages(t0, t1)) {
    success = false;

  } else if (t0.is_definition_usage() && t0.usage == t1.usage) {
    success = simplify(t0.members, t1.members, rev);

  } else {
    DestructuredSimplifier simplifier(*this);
    DestructuredMemberVisitor<Type*> visitor(store, simplifier);
    success = visitor.expand_members(lhs, rhs, t0, t1, rev);
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

bool Simplifier::simplify(const types::Class& t0, const types::Class& t1, bool rev) {
  return simplify(t0.source, t1.source, rev);
}

bool Simplifier::simplify(Type* lhs, Type* rhs, const types::Scheme&, const types::Scheme&, bool rev) {
  EquivalenceRelation equiv;
  TypeRelation relate(equiv, store);
  return relate.related_entry(lhs, rhs, rev);
}

bool Simplifier::simplify(const types::Subscript& t0, const types::Subscript& t1, bool rev) {
  if (t0.subscripts.size() != t1.subscripts.size()) {
    return false;
  }
  for (int64_t i = 0; i < int64_t(t0.subscripts.size()); i++) {
    const auto& args0 = t0.subscripts[i];
    const auto& args1 = t1.subscripts[i];

    if (args0.method != args1.method) {
      return false;
    }

    if (!simplify(args0.arguments, args1.arguments, !rev)) {
      return false;
    }
  }
  return simplify(t0.outputs, t1.outputs, rev);
}

bool Simplifier::simplify(Type* lhs, Type* rhs, const types::Tuple& t0, const types::Tuple& t1, bool rev) {
  bool success = simplify(t0.members, t1.members, rev);
  check_emplace_simplification_failure(success, lhs, rhs);
  return success;
}

bool Simplifier::simplify(const TypePtrs& t0, const TypePtrs& t1, bool rev) {
  const auto sz0 = t0.size();
  if (sz0 != t1.size()) {
    return false;
  }
  push_make_type_equations(t0, t1, sz0, rev);
  return true;
}

void Simplifier::push_make_type_equations(const TypePtrs& t0, const TypePtrs& t1, int64_t num, bool rev) {
  assert(num >= 0 && num <= int64_t(t0.size()) && num <= int64_t(t1.size()) && "out of bounds read.");
  for (int64_t i = 0; i < num; i++) {
    push_make_type_equation(t0[i], t1[i], rev);
  }
}

void Simplifier::push_type_equation(const TypeEquation& eq) {
  unifier.substitution->push_type_equation(eq);
}

void Simplifier::push_make_type_equation(Type* t0, Type* t1, bool rev) {
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

void Simplifier::check_emplace_simplification_failure(bool success, const Type* lhs, const Type* rhs) {
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
