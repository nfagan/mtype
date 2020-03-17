#include "type_equality.hpp"
#include "typing.hpp"
#include "debug.hpp"
#include <cassert>

namespace mt {

DebugTypePrinter TypeEquality::type_printer() const {
  return DebugTypePrinter(store, string_registry);
}

Type::Tag TypeEquality::type_of(const TypeHandle& handle) const {
  return store.at(handle).tag;
}

bool TypeEquality::element_wise_equivalence(const std::vector<TypeHandle>& a, const std::vector<TypeHandle>& b) const {
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
    default:
      assert(false && "Unhandled type equivalence.");
  }
}

bool TypeEquality::equivalence_different_types(const TypeHandle& a, const TypeHandle& b) const {
  const auto& value_a = store.at(a);
  const auto& value_b = store.at(b);

  if (value_a.is_destructured_tuple()) {
    return equivalence_different_types(value_a.destructured_tuple, b);

  } else if (value_b.is_destructured_tuple()) {
    return equivalence_different_types(value_b.destructured_tuple, a);

  } else {
//    std::cout << "Non equivalent: " << value_a.tag << ", " << value_b.tag << std::endl;
    return false;
  }
}

bool TypeEquality::equivalence(const types::Scalar& a, const types::Scalar& b) const {
  return a.identifier.name == b.identifier.name;
}

bool TypeEquality::equivalence(const types::List& a, const types::List& b) const {
  return element_wise_equivalence(a.pattern, b.pattern);
}

bool TypeEquality::equivalence(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const {
  if (types::DestructuredTuple::mismatching_definition_usages(a, b)) {
    return false;

  } else if (a.is_definition_usage() && a.usage == b.usage) {
    return equivalence_same_definition_usage(a, b);

  } else {
    return equivalence_expanding_members(a, b);
  }
}

bool TypeEquality::equivalence_same_definition_usage(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const {
  return element_wise_equivalence(a.members, b.members);
}

bool TypeEquality::equivalence_expanding_members(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const {
  const int64_t num_a = a.members.size();
  const int64_t num_b = b.members.size();

  int64_t ia = 0;
  int64_t ib = 0;

  while (ia < num_a && ib < num_b) {
    bool res = equivalence_recurse_tuple(a, b, &ia, &ib);
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
//    std::cout << "ia: " << ia << "; ib: " << ib << "; num a: " << num_a << "; num_b: " << num_b << std::endl;
    return false;
  }
}

bool TypeEquality::equivalence_recurse_tuple(const types::DestructuredTuple& a,
                                             const types::DestructuredTuple& b,
                                             int64_t* ia, int64_t* ib) const {
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
    return equivalence_subrecurse_tuple(a, b, va.destructured_tuple, ia, ib);

  } else if (vb.is_destructured_tuple()) {
    return equivalence_subrecurse_tuple(b, a, vb.destructured_tuple, ib, ia);

  } else {
    (*ia)++;
    (*ib)++;

    return equivalence(mem_a, mem_b);
  }
}

bool TypeEquality::equivalence_subrecurse_tuple(const types::DestructuredTuple& a,
                                                const types::DestructuredTuple& b,
                                                const types::DestructuredTuple& sub_a,
                                                int64_t* ia,
                                                int64_t* ib) const {
  int64_t ia_child = 0;
  const int64_t num_sub_a = sub_a.size();
  int64_t expect_match = num_sub_a;

  if (a.is_value_usage() && sub_a.is_outputs()) {
    expect_match = 1;
  }

  bool success = true;

  while (success && ia_child < expect_match && ia_child < num_sub_a && *ib < b.size()) {
    success = equivalence_recurse_tuple(sub_a, b, &ia_child, ib);
  }

  (*ia)++;

  return success && ia_child == expect_match;
}

bool TypeEquality::equivalence_subrecurse_list(const types::List& a, int64_t* ia,
  const types::DestructuredTuple& b, const TypeHandle& mem_b) const {

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
      success = equivalence_subrecurse_list(a, ia, sub_b, sub_b.members[ib++]);
    }

    return success && ib == expect_num_b;

  } else {
    *ia = (*ia + 1) % a.size();
    return equivalence(mem_a, mem_b);
  }
}

bool TypeEquality::match_list(const types::List& a, const types::DestructuredTuple& b, int64_t* ib) const {
  int64_t ia = 0;
  bool success = true;

  while (success && ia < a.size() && *ib < b.size()) {
    success = equivalence_subrecurse_list(a, &ia, b, b.members[(*ib)++]);
  }

  return success && (a.size() == 0 || (ia == 0 && *ib == b.size()));
}

bool TypeEquality::equivalence_different_types(const types::DestructuredTuple& a, const TypeHandle& b) const {
  return a.size() == 1 && equivalence(a.members[0], b);
}

bool TypeEquality::TypeEquivalenceComparator::equivalence(const TypeHandle& a, const TypeHandle& b) const {
  return type_eq.equivalence(a, b);
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
  }

  const auto& args_a = type_eq.store.at(a.inputs);
  const auto& args_b = type_eq.store.at(b.inputs);

  assert(args_a.is_destructured_tuple() && args_b.is_destructured_tuple());

  const auto& tup_a = args_a.destructured_tuple;
  const auto& tup_b = args_b.destructured_tuple;

  //  Important -- allow for type equivalence first.
  if (type_eq.equivalence(tup_a, tup_b)) {
    return false;
  }

  if (tup_a.members.size() != tup_b.members.size()) {
    return tup_a.members.size() < tup_b.members.size();
  }

  const int64_t num_members = tup_a.members.size();

  for (int64_t i = 0; i < num_members; i++) {
    if (!type_eq.equivalence(tup_a.members[i], tup_b.members[i])) {
      return tup_a.members[i] < tup_b.members[i];
    }
  }

  return false;
}

}