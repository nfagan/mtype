#include "type_relation.hpp"
#include "debug.hpp"
#include "destructured_member_visitor.hpp"
#include "union_member_visitor.hpp"
#include <cassert>

namespace mt {

class UnionVisitor : public UnionMemberVisitor::Predicate {
public:
  explicit UnionVisitor(const TypeRelation& relation) : relation(relation) {
    //
  }

  bool operator()(Type* lhs, Type* rhs, bool rev) const override {
    return relation.related_entry(lhs, rhs, rev);
  }

  const TypeRelation& relation;
};

class DestructuredVisitor : public DestructuredMemberVisitor<const Type*>::Predicate {
public:
  explicit DestructuredVisitor(const TypeRelation& relation) : relation(relation) {
    //
  }
  bool predicate(const Type* a, const Type* b, bool rev) const override {
    return relation.related(a, b, rev);
  }
  bool parameters(const Type* lhs, const Type* rhs, const types::Parameters&,
                  const types::DestructuredTuple&, int64_t, bool rev) const override {
    return relation.related(lhs, rhs, rev);
  }

  const TypeRelation& relation;
};

DebugTypePrinter TypeRelation::type_printer() const {
  return DebugTypePrinter();
}

bool TypeRelation::related_element_wise(const TypePtrs& a, const TypePtrs& b, bool rev) const {
  return related_element_wise(a, a.size(), b, b.size(), rev);
}

bool TypeRelation::related_element_wise(const TypePtrs& a, const int64_t num_a,
                                        const TypePtrs& b, const int64_t num_b, bool rev) const {
  if (num_a != num_b) {
    return false;
  }
  for (int64_t i = 0; i < num_a; i++) {
    if (!related(a[i], b[i], rev)) {
      return false;
    }
  }
  return true;
}

bool TypeRelation::related_entry(const Type* a, const Type* b, bool rev) const {
  return related(a, b, rev);
}

bool TypeRelation::related(const Type* a, const Type* b, bool rev) const {
  if (a->tag == b->tag) {
    return related_same_types(a, b, rev);
  } else {
    return related_different_types(a, b, rev);
  }
}

bool TypeRelation::related_same_types(const Type* a, const Type* b, bool rev) const {
  switch (a->tag) {
    case Type::Tag::scalar:
      return related(a, b, MT_SCALAR_REF(*a), MT_SCALAR_REF(*b), rev);
    case Type::Tag::destructured_tuple:
      return related(a, b, MT_DT_REF(*a), MT_DT_REF(*b), rev);
    case Type::Tag::list:
      return related(MT_LIST_REF(*a), MT_LIST_REF(*b), rev);
    case Type::Tag::tuple:
      return related(MT_TUPLE_REF(*a), MT_TUPLE_REF(*b), rev);
    case Type::Tag::union_type:
      return related(MT_UNION_REF(*a), MT_UNION_REF(*b), rev);
    case Type::Tag::abstraction:
      return related(MT_ABSTR_REF(*a), MT_ABSTR_REF(*b), rev);
    case Type::Tag::variable:
      return true;
    case Type::Tag::scheme:
      return related(MT_SCHEME_REF(*a), MT_SCHEME_REF(*b), rev);
    case Type::Tag::class_type:
      return related(MT_CLASS_REF(*a), MT_CLASS_REF(*b), rev);
    case Type::Tag::alias:
      return related(MT_ALIAS_REF(*a).source, MT_ALIAS_REF(*b).source, rev);
    default:
      type_printer().show2(a, b);
      std::cout << std::endl;
      assert(false && "Unhandled type equivalence.");
      return false;
  }
}

bool TypeRelation::related_different_types(const Type* a, const Type* b, bool rev) const {
  if (a->is_variable() || b->is_variable()) {
    return true;
  }

  if (a->is_alias()) {
    return related(MT_ALIAS_REF(*a).source, b, rev);

  } else if (b->is_alias()) {
    return related(a, MT_ALIAS_REF(*b).source, !rev);

  } else if (a->is_destructured_tuple()) {
    return related_different_types(MT_DT_REF(*a), b, rev);

  } else if (b->is_destructured_tuple()) {
    return related_different_types(MT_DT_REF(*b), a, !rev);

  } else if (a->is_union()) {
    return related_different_types(MT_UNION_REF(*a), b, rev);

  } else if (b->is_union()) {
    return related_different_types(MT_UNION_REF(*b), a, !rev);

  } else if (a->is_list()) {
    return related_different_types(MT_LIST_REF(*a), b, rev);

  } else if (b->is_list()) {
    return related_different_types(MT_LIST_REF(*b), a, !rev);

  } else if (a->is_scheme()) {
    return related_different_types(MT_SCHEME_REF(*a), b, rev);

  } else if (b->is_scheme()) {
    return related_different_types(MT_SCHEME_REF(*b), a, !rev);

  } else {
    return false;
  }
}

bool TypeRelation::related(const Type* lhs, const Type* rhs,
                           const types::Scalar&, const types::Scalar&, bool rev) const {
  return relationship.related(lhs, rhs, rev);
}

bool TypeRelation::related_list(const TypePtrs& a, const TypePtrs& b,
                                int64_t* ia, int64_t* ib,
                                int64_t num_a, int64_t num_b, bool rev) const {

  if (*ia == num_a) {
    return *ib == num_b;

  } else if (*ib == num_b) {
    return *ia == num_a;

  } else {
    assert(*ia < int64_t(a.size()) && *ib < int64_t(b.size()));
    assert(num_a <= int64_t(a.size()) && num_b <= int64_t(b.size()));
  }

  const auto& va = a[*ia];
  const auto& vb = b[*ib];

  if (va->is_list()) {
    int64_t new_ia = 0;
    const auto& list_a = MT_LIST_REF(*va);
    bool success = related_list(list_a.pattern, b, &new_ia, ib, list_a.size(), num_b, rev);
    (*ia)++;
    return success;

  } else if (vb->is_list()) {
    int64_t new_ib = 0;
    const auto& list_b = MT_LIST_REF(*vb);
    bool success = related_list(a, list_b.pattern, ia, &new_ib, num_a, list_b.size(), rev);
    (*ib)++;
    return success;

  } else if (va->is_destructured_tuple()) {
    bool success = related_list_sub_tuple(MT_DT_REF(*va), b, ib, num_b, rev);
    (*ia)++;
    return success;

  } else if (vb->is_destructured_tuple()) {
    bool success = related_list_sub_tuple(MT_DT_REF(*vb), a, ia, num_a, !rev);
    (*ib)++;
    return success;

  } else {
    (*ia)++;
    (*ib)++;

    return related(va, vb, rev);
  }
}

bool TypeRelation::related_list_sub_tuple(const DT& tup_a, const TypePtrs& b, int64_t* ib,
                                          int64_t num_b, bool rev) const {
  const int64_t use_num_a = tup_a.is_outputs() ? std::min(int64_t(1), tup_a.size()) : tup_a.size();
  int64_t new_ia = 0;
  return related_list(tup_a.members, b, &new_ia, ib, use_num_a, num_b, rev);
}

bool TypeRelation::related(const types::List& a, const types::List& b, bool rev) const {
  int64_t ia = 0;
  int64_t ib = 0;
  return related_list(a.pattern, b.pattern, &ia, &ib, a.size(), b.size(), rev);
}

bool TypeRelation::related(const types::Tuple& a, const types::Tuple& b, bool rev) const {
  return related_element_wise(a.members, b.members, rev);
}

bool TypeRelation::related(const types::Union& a, const types::Union& b, bool rev) const {
  UnionVisitor relater(*this);
  UnionMemberVisitor visitor(&relater);
  return visitor(a, b, rev);
}

bool TypeRelation::related(const types::Scheme& a, const types::Scheme& b, bool rev) const {
//  assert(false && "Scheme relation not yet handled.");
  return related(a.type, b.type, rev);
}

bool TypeRelation::related(const types::Class& a, const types::Class& b, bool rev) const {
  return relationship.related(&a, &b, rev);
}

bool TypeRelation::related(const types::Abstraction& a, const types::Abstraction& b, bool rev) const {
  using Type = types::Abstraction::Kind;

  if (a.kind != b.kind) {
    return false;
  }
  if (a.kind == Type::binary_operator && a.binary_operator != b.binary_operator) {
    return false;

  } else if (a.kind == Type::unary_operator && a.unary_operator != b.unary_operator) {
    return false;

  } else if (a.kind == Type::subscript_reference && a.subscript_method != b.subscript_method) {
    return false;

  } else if (a.kind == Type::function && a.name != b.name) {
    return false;

  } else if (a.kind == Type::concatenation && a.concatenation_direction != b.concatenation_direction) {
    return false;
  }

  return related(a.inputs, b.inputs, !rev) && related(a.outputs, b.outputs, rev);
}

bool TypeRelation::related(const Type* lhs, const Type* rhs, const DT& a, const DT& b, bool rev) const {
  if (types::DestructuredTuple::mismatching_definition_usages(a, b)) {
    return false;

  } else if (a.is_definition_usage() && a.usage == b.usage) {
    return related_element_wise(a.members, b.members, rev);

  } else {
    DestructuredVisitor relater(*this);
    DestructuredMemberVisitor<const Type*> visitor(store, relater);
    return visitor.expand_members(lhs, rhs, a, b, rev);
  }
}

bool TypeRelation::related_different_types(const types::Scheme& a, const Type* b, bool rev) const {
  return related(a.type, b, rev);
}

bool TypeRelation::related_different_types(const types::DestructuredTuple& a, const Type* b, bool rev) const {
  const auto sz = a.size();
  if (sz == 0) {
    return b->is_list();
  } else if (sz == 1) {
    return related(a.members[0], b, rev);
  } else {
    return false;
  }
}

bool TypeRelation::related_different_types(const types::List& a, const Type* b, bool rev) const {
  return a.size() == 1 && related(a.pattern[0], b, rev);
}

bool TypeRelation::related_different_types(const types::Union& a, const Type* b, bool rev) const {
  for (const auto& mem : a.members) {
    if (related(mem, b, rev)) {
      return true;
    }
  }
  return false;
}

}