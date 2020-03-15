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
  return a.usage == b.usage && element_wise_equivalence(a.pattern, b.pattern);
}

bool TypeEquality::equivalence(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const {
  if (a.usage == b.usage) {
    return equivalence_same_usage(a, b);

  } else if (types::DestructuredTuple::mismatching_definition_usages(a, b)) {
    return false;

  } else {
    return equivalence_different_usage(a, b);
  }
}

bool TypeEquality::equivalence_same_usage(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const {
  return element_wise_equivalence(a.members, b.members);
}

bool TypeEquality::equivalence_different_usage(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const {
  using Tag = Type::Tag;

  const int64_t num_a = a.members.size();
  const int64_t num_b = b.members.size();

  int64_t ia = 0;
  int64_t ib = 0;

  while (ia < num_a && ib < num_b) {
    const auto& mem_a = a.members[ia];
    const auto& mem_b = b.members[ib];

    const auto& va = store.at(mem_a);
    const auto& vb = store.at(mem_b);

    const auto ta = va.tag;
    const auto tb = vb.tag;

    bool equiv;
    int64_t num_incr_a = 1;
    int64_t num_incr_b = 1;

    if (ta == tb) {
      equiv = equivalence_same_types(mem_a, mem_b);

    } else if (va.is_list()) {
      equiv = match_list(va.list, b.members, ib, &num_incr_b);

    } else if (vb.is_list()) {
      equiv = match_list(vb.list, a.members, ia, &num_incr_a);

    } else {
//      std::cout << "Diff usage: " << std::endl;
//      type_printer().show2(a, b);
//      std::cout << std::endl;

      equiv = equivalence(mem_a, mem_b);
    }

    if (!equiv) {
      return false;
    }

    ia += num_incr_a;
    ib += num_incr_b;
  }

  return ia == num_a && ib == num_b;
}

bool TypeEquality::match_list(const types::List& a, const std::vector<TypeHandle>& b, int64_t ib, int64_t* num_incr_b) const {
  assert(ib < b.size());

  int64_t ia = 0;
  const int64_t num_a = a.size();
  const int64_t num_b = b.size();
  const int64_t b0 = ib;

  while (ia < num_a && ib < num_b) {
    const auto& pat_a = a.pattern[ia];
    const auto& mem_b = b[ib];

    if (equivalence(pat_a, mem_b)) {
      ia = (ia + 1) % num_a;
      ib++;

    } else {
      break;
    }
  }

  //  True if all pattern members of `a` were visited.
  const bool match = num_a == 0 || (ia == 0 && ib > b0);

  if (match) {
    *num_incr_b = (ib - b0);
  }

  return match;
}

bool TypeEquality::equivalence_different_types(const types::DestructuredTuple& a, const TypeHandle& b) const {
  using Use = types::DestructuredTuple::Usage;
  const auto num_members = a.members.size();

  if (num_members == 1 || (num_members > 0 && a.usage == Use::definition_outputs)) {
    return equivalence(a.members[0], b);

  } else {
    std::cout << "Unexpected different types:" << std::endl;
    type_printer().show2(a, b);
    std::cout << std::endl;
    assert(false && "Unhandled equivalence.");
    return false;
  }
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
  if (type_eq.equivalence_different_usage(tup_a, tup_b)) {
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