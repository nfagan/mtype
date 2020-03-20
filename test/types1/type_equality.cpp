#include "type_equality.hpp"
#include "typing.hpp"
#include "debug.hpp"
#include "member_visitor.hpp"
#include <cassert>

namespace mt {

DebugTypePrinter TypeEquality::type_printer() const {
  return DebugTypePrinter(store, string_registry);
}

Type::Tag TypeEquality::type_of(const TypeHandle& handle) const {
  return store.at(handle).tag;
}

bool TypeEquality::element_wise_equivalence(const TypeHandles& a, const TypeHandles& b) const {
  const int64_t size_a = a.size();
  if (size_a != b.size()) {
    return false;
  }
  for (int64_t i = 0; i < size_a; i++) {
    if (!equivalence(a[i], b[i])) {
      return false;
    }
  }
  return true;
}

bool TypeEquality::equivalence(const TypeHandle& a, const TypeHandle& b) const {
  if (type_of(a) == type_of(b)) {
    return equivalence_same_types(a, b);
  } else {
    return equivalence_different_types(a, b);
  }
}

bool TypeEquality::equivalence_same_types(const TypeHandle& a, const TypeHandle& b) const {
  const auto& value_a = store.at(a);
  const auto& value_b = store.at(b);

  switch (value_a.tag) {
    case Type::Tag::scalar:
      return equivalence(value_a.scalar, value_b.scalar);
    case Type::Tag::destructured_tuple:
      return equivalence(value_a.destructured_tuple, value_b.destructured_tuple);
    case Type::Tag::list:
      return equivalence(value_a.list, value_b.list);
    case Type::Tag::tuple:
      return equivalence(value_a.tuple, value_b.tuple);
    case Type::Tag::variable:
      return true;
    default:
      type_printer().show2(a, b);
      std::cout << std::endl;
      assert(false && "Unhandled type equivalence.");
      return false;
  }
}

bool TypeEquality::equivalence_different_types(const TypeHandle& a, const TypeHandle& b) const {
  const auto& value_a = store.at(a);
  const auto& value_b = store.at(b);

  if (value_a.is_variable() || value_b.is_variable()) {
    return true;
  }

  if (value_a.is_destructured_tuple()) {
    return equivalence_different_types(value_a.destructured_tuple, b);

  } else if (value_b.is_destructured_tuple()) {
    return equivalence_different_types(value_b.destructured_tuple, a);

  } else if (value_a.is_list()) {
    return equivalence_different_types(value_a.list, b);

  } else if (value_b.is_list()) {
    return equivalence_different_types(value_b.list, a);

  } else {
    return false;
  }
}

bool TypeEquality::equivalence(const types::Scalar& a, const types::Scalar& b) const {
  return a.identifier.name == b.identifier.name;
}

bool TypeEquality::equivalence_list(const TypeHandles& a, const TypeHandles& b, int64_t* ia, int64_t* ib,
  int64_t num_a, int64_t num_b) const {

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
    bool success = equivalence_list(va.list.pattern, b, &new_ia, ib, va.list.size(), num_b);
    (*ia)++;
    return success;

  } else if (vb.is_list()) {
    int64_t new_ib = 0;
    bool success = equivalence_list(a, vb.list.pattern, ia, &new_ib, num_a, vb.list.size());
    (*ib)++;
    return success;

  } else if (va.is_destructured_tuple()) {
    bool success = equivalence_list_sub_tuple(va.destructured_tuple, b, ib, num_b);
    (*ia)++;
    return success;

  } else if (vb.is_destructured_tuple()) {
    bool success = equivalence_list_sub_tuple(vb.destructured_tuple, a, ia, num_a);
    (*ib)++;
    return success;

  } else {
    (*ia)++;
    (*ib)++;

    return equivalence(mem_a, mem_b);
  }
}

bool TypeEquality::equivalence_list_sub_tuple(const DT& tup_a, const TypeHandles& b, int64_t* ib, int64_t num_b) const {
  const int64_t use_num_a = tup_a.is_outputs() ? std::min(int64_t(1), tup_a.size()) : tup_a.size();
  int64_t new_ia = 0;
  return equivalence_list(tup_a.members, b, &new_ia, ib, use_num_a, num_b);
}

bool TypeEquality::equivalence(const types::List& a, const types::List& b) const {
  int64_t ia = 0;
  int64_t ib = 0;
  return equivalence_list(a.pattern, b.pattern, &ia, &ib, a.size(), b.size());
}

bool TypeEquality::equivalence(const types::Tuple& a, const types::Tuple& b) const {
  return element_wise_equivalence(a.members, b.members);
}

bool TypeEquality::equivalence(const DT& a, const DT& b) const {
  if (types::DestructuredTuple::mismatching_definition_usages(a, b)) {
    return false;

  } else if (a.is_definition_usage() && a.usage == b.usage) {
    return element_wise_equivalence(a.members, b.members);

  } else {
    return DestructuredMemberVisitor{store, DestructuredComparator{*this}}.expand_members(a, b);
  }
}

bool TypeEquality::equivalence_different_types(const types::DestructuredTuple& a, const TypeHandle& b) const {
  return a.size() == 1 && equivalence(a.members[0], b);
}

bool TypeEquality::equivalence_different_types(const types::List& a, const TypeHandle& b) const {
  return a.size() == 1 && equivalence(a.pattern[0], b);
}

bool TypeEquality::TypeEquivalenceComparator::operator()(const TypeHandle& a, const TypeHandle& b) const {
  if (type_eq.equivalence(a, b)) {
    return false;
  }

  return a < b;
}

bool TypeEquality::ArgumentComparator::operator()(const types::Abstraction& a, const types::Abstraction& b) const {
  using Type = types::Abstraction::Type;

  if (a.type != b.type) {
    return a.type < b.type;
  }

  if (a.type == Type::binary_operator && a.binary_operator != b.binary_operator) {
    return a.binary_operator < b.binary_operator;

  } else if (a.type == Type::unary_operator && a.unary_operator != b.unary_operator) {
    return a.unary_operator < b.unary_operator;

  } else if (a.type == Type::subscript_reference && a.subscript_method != b.subscript_method) {
    return a.subscript_method < b.subscript_method;

  } else if (a.type == Type::function && a.name != b.name) {
    return a.name < b.name;

  } else if (a.type == Type::concatenation && a.concatenation_direction != b.concatenation_direction) {
    return a.concatenation_direction < b.concatenation_direction;
  }

  const auto& args_a = type_eq.store.at(a.inputs);
  const auto& args_b = type_eq.store.at(b.inputs);

  assert(args_a.is_destructured_tuple() && args_b.is_destructured_tuple());

  const auto& tup_a = args_a.destructured_tuple;
  const auto& tup_b = args_b.destructured_tuple;

  return !type_eq.equivalence(tup_a, tup_b);
}

}