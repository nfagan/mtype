#include "type_relation.hpp"
#include "debug.hpp"
#include "member_visitor.hpp"
#include <cassert>

namespace mt {

class DestructuredVisitor : public DestructuredMemberVisitor::Predicate {
public:
  explicit DestructuredVisitor(const TypeRelation& relation) : relation(relation) {
    //
  }
  bool predicate(const TypeHandle& a, const TypeHandle& b, bool rev) const override {
    return relation.related(a, b, rev);
  }
  virtual bool parameters(TypeRef lhs, TypeRef rhs, const types::Parameters& a,
                          const types::DestructuredTuple& b, int64_t offset_b, bool rev) const override {
    return relation.related(lhs, rhs, rev);
  }

  const TypeRelation& relation;
};

DebugTypePrinter TypeRelation::type_printer() const {
  return DebugTypePrinter(store);
}

Type::Tag TypeRelation::type_of(const TypeHandle& handle) const {
  return store.at(handle).tag;
}

bool TypeRelation::element_wise_related(const TypeHandles& a, const TypeHandles& b, bool rev) const {
  const int64_t size_a = a.size();
  if (size_a != b.size()) {
    return false;
  }
  for (int64_t i = 0; i < size_a; i++) {
    if (!related(a[i], b[i], rev)) {
      return false;
    }
  }
  return true;
}

bool TypeRelation::related_entry(const TypeHandle& a, const TypeHandle& b, bool rev) const {
  return related(a, b, rev);
}

bool TypeRelation::related(const TypeHandle& a, const TypeHandle& b, bool rev) const {
  if (type_of(a) == type_of(b)) {
    return related_same_types(a, b, rev);
  } else {
    return related_different_types(a, b, rev);
  }
}

bool TypeRelation::related_same_types(const TypeHandle& a, const TypeHandle& b, bool rev) const {
  const auto& value_a = store.at(a);
  const auto& value_b = store.at(b);

  switch (value_a.tag) {
    case Type::Tag::scalar:
      return related(a, b, value_a.scalar, value_b.scalar, rev);
    case Type::Tag::destructured_tuple:
      return related(a, b, value_a.destructured_tuple, value_b.destructured_tuple, rev);
    case Type::Tag::list:
      return related(value_a.list, value_b.list, rev);
    case Type::Tag::tuple:
      return related(value_a.tuple, value_b.tuple, rev);
    case Type::Tag::abstraction:
      return related(value_a.abstraction, value_b.abstraction, rev);
    case Type::Tag::variable:
      return true;
    case Type::Tag::scheme:
      return related(value_a.scheme, value_b.scheme, rev);
    default:
      type_printer().show2(a, b);
      std::cout << std::endl;
      assert(false && "Unhandled type equivalence.");
      return false;
  }
}

bool TypeRelation::related_different_types(const TypeHandle& a, const TypeHandle& b, bool rev) const {
  const auto& value_a = store.at(a);
  const auto& value_b = store.at(b);

  if (value_a.is_variable() || value_b.is_variable()) {
    return true;
  }

  if (value_a.is_destructured_tuple()) {
    return related_different_types(value_a.destructured_tuple, b, rev);

  } else if (value_b.is_destructured_tuple()) {
    return related_different_types(value_b.destructured_tuple, a, !rev);

  } else if (value_a.is_list()) {
    return related_different_types(value_a.list, b, rev);

  } else if (value_b.is_list()) {
    return related_different_types(value_b.list, a, !rev);

  } else if (value_a.is_scheme()) {
    return related_different_types(value_a.scheme, b, rev);

  } else if (value_b.is_scheme()) {
    return related_different_types(value_b.scheme, a, !rev);

  } else {
    return false;
  }
}

bool TypeRelation::related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const {
  return relationship.related(lhs, rhs, a, b, rev);
}

bool TypeRelation::related_list(const TypeHandles& a, const TypeHandles& b, int64_t* ia, int64_t* ib,
                                int64_t num_a, int64_t num_b, bool rev) const {

  if (*ia == num_a) {
    return *ib == num_b;

  } else if (*ib == num_b) {
    return *ia == num_a;

  } else {
    assert(*ia < a.size() && *ib < b.size());
    assert(num_a <= a.size() && num_b <= b.size());
  }

  const auto& mem_a = a[*ia];
  const auto& mem_b = b[*ib];

  const auto& va = store.at(mem_a);
  const auto& vb = store.at(mem_b);

  if (va.is_list()) {
    int64_t new_ia = 0;
    bool success = related_list(va.list.pattern, b, &new_ia, ib, va.list.size(), num_b, rev);
    (*ia)++;
    return success;

  } else if (vb.is_list()) {
    int64_t new_ib = 0;
    bool success = related_list(a, vb.list.pattern, ia, &new_ib, num_a, vb.list.size(), rev);
    (*ib)++;
    return success;

  } else if (va.is_destructured_tuple()) {
    bool success = related_list_sub_tuple(va.destructured_tuple, b, ib, num_b, rev);
    (*ia)++;
    return success;

  } else if (vb.is_destructured_tuple()) {
    bool success = related_list_sub_tuple(vb.destructured_tuple, a, ia, num_a, !rev);
    (*ib)++;
    return success;

  } else {
    (*ia)++;
    (*ib)++;

    return related(mem_a, mem_b, rev);
  }
}

bool TypeRelation::related_list_sub_tuple(const DT& tup_a, const TypeHandles& b, int64_t* ib, int64_t num_b, bool rev) const {
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
  return element_wise_related(a.members, b.members, rev);
}

bool TypeRelation::related(const types::Scheme& a, const types::Scheme& b, bool rev) const {
//  assert(false && "Scheme relation not yet handled.");
  return related(a.type, b.type, rev);
}

bool TypeRelation::related(const types::Abstraction& a, const types::Abstraction& b, bool rev) const {
  using Type = types::Abstraction::Type;

  if (a.type != b.type) {
    return false;
  }
  if (a.type == Type::binary_operator && a.binary_operator != b.binary_operator) {
    return false;

  } else if (a.type == Type::unary_operator && a.unary_operator != b.unary_operator) {
    return false;

  } else if (a.type == Type::subscript_reference && a.subscript_method != b.subscript_method) {
    return false;

  } else if (a.type == Type::function && a.name != b.name) {
    return false;

  } else if (a.type == Type::concatenation && a.concatenation_direction != b.concatenation_direction) {
    return false;
  }

  return related(a.inputs, b.inputs, !rev) && related(a.outputs, b.outputs, rev);
}

bool TypeRelation::related(TypeRef lhs, TypeRef rhs, const DT& a, const DT& b, bool rev) const {
  if (types::DestructuredTuple::mismatching_definition_usages(a, b)) {
    return false;

  } else if (a.is_definition_usage() && a.usage == b.usage) {
    return element_wise_related(a.members, b.members, rev);

  } else {
    return DestructuredMemberVisitor{store, DestructuredVisitor{*this}}.expand_members(lhs, rhs, a, b, rev);
  }
}

bool TypeRelation::related_different_types(const types::Scheme& a, const TypeHandle& b, bool rev) const {
  return related(a.type, b, rev);
}

bool TypeRelation::related_different_types(const types::DestructuredTuple& a, const TypeHandle& b, bool rev) const {
  return a.size() == 1 && related(a.members[0], b, rev);
}

bool TypeRelation::related_different_types(const types::List& a, const TypeHandle& b, bool rev) const {
  return a.size() == 1 && related(a.pattern[0], b, rev);
}

bool TypeRelation::TypeRelationComparator::operator()(const TypeHandle& a, const TypeHandle& b) const {
  if (type_relation.related(a, b, false)) {
    return false;
  }

  return a < b;
}

bool TypeRelation::ArgumentComparator::operator()(const types::Abstraction& a, const types::Abstraction& b) const {
  using Type = types::Abstraction::Type;

  types::Abstraction::HeaderCompare compare{};
  const auto header_result = compare(a, b);
  if (header_result == -1) {
    return true;
  } else if (header_result == 1) {
    return false;
  }

  const auto& args_a = type_relation.store.at(a.inputs);
  const auto& args_b = type_relation.store.at(b.inputs);

  if (args_a.tag != args_b.tag) {
    return args_a.tag < args_b.tag;
  }

  assert(args_a.is_destructured_tuple() && args_b.is_destructured_tuple());

  const auto& tup_a = args_a.destructured_tuple;
  const auto& tup_b = args_b.destructured_tuple;

#if 0
  std::cout << "Comparing args for type: ";
  type_relation.type_printer().show(a);
  std::cout << std::endl;
  type_relation.type_printer().show2(tup_a, tup_b);
  bool related = type_relation.related(tup_a, tup_b, false);
  std::cout << "Related ? " << related << std::endl << "===\n";
  return !related;

#else
  return !type_relation.related(a.inputs, b.inputs, tup_a, tup_b, false);
#endif
}

bool TypeRelation::NameComparator::operator()(const types::Abstraction& a, const types::Abstraction& b) const {
  types::Abstraction::HeaderCompare compare{};
  return compare(a, b) == -1;
}


}