#pragma once

#include "type.hpp"
#include "type_store.hpp"

namespace mt {

class TypeVisitor;
class StringRegistry;
class DebugTypePrinter;

class TypeEquality {
public:
  struct TypeEquivalenceComparator {
    explicit TypeEquivalenceComparator(const TypeEquality& type_eq) : type_eq(type_eq) {
      //
    }

    bool operator()(const TypeHandle& a, const TypeHandle& b) const;
    bool equivalence(const TypeHandle& a, const TypeHandle& b) const;

    const TypeEquality& type_eq;
  };

  struct ArgumentComparator {
    explicit ArgumentComparator(const TypeEquality& type_eq) : type_eq(type_eq) {
      //
    }

    bool operator()(const types::Abstraction& a, const types::Abstraction& b) const;
    const TypeEquality& type_eq;
  };

public:
  TypeEquality(const TypeStore& store, const StringRegistry& string_registry) :
  store(store), string_registry(string_registry) {
    //
  }

private:
  bool equivalence(const TypeHandle& a, const TypeHandle& b) const;
  bool element_wise_equivalence(const std::vector<TypeHandle>& a, const std::vector<TypeHandle>& b) const;
  bool equivalence_different_types(const TypeHandle& a, const TypeHandle& b) const;
  bool equivalence_same_types(const TypeHandle& a, const TypeHandle& b) const;

  bool equivalence(const types::Scalar& a, const types::Scalar& b) const;
  bool equivalence(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const;
  bool equivalence(const types::List& a, const types::List& b) const;
  bool equivalence(const types::Tuple& a, const types::Tuple& b) const;

  bool equivalence_different_types(const types::DestructuredTuple& a, const TypeHandle& b) const;
  bool equivalence_different_types(const types::List& a, const TypeHandle& b) const;
  bool match_list(const types::List& a, const types::DestructuredTuple& b, int64_t* ib) const;

  bool equivalence_same_definition_usage(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const;
  bool equivalence_expanding_members(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const;
  bool equivalence_recurse_tuple(const types::DestructuredTuple& a,
                                 const types::DestructuredTuple& b,
                                 int64_t* ia, int64_t* ib) const;
  bool equivalence_subrecurse_tuple(const types::DestructuredTuple& a,
                                    const types::DestructuredTuple& b,
                                    int64_t* ib, int64_t expect_match) const;
  bool equivalence_subrecurse_list(const types::List& a, int64_t* ia,
    const types::DestructuredTuple& b, const TypeHandle& mem_b) const;

  bool equivalence_list(const TypeHandles& a, const TypeHandles& b, int64_t* ia, int64_t* ib,
    int64_t num_a, int64_t num_b) const;
  bool equivalence_list_sub_tuple(const types::DestructuredTuple& tup_a,
                                  const TypeHandles& b, int64_t* ib, int64_t num_b) const;

  DebugTypePrinter type_printer() const;

  int64_t expect_to_match(const types::DestructuredTuple& parent, const types::DestructuredTuple& child) const;

private:
  Type::Tag type_of(const TypeHandle& handle) const;

private:
  const TypeStore& store;
  const StringRegistry& string_registry;
};

}