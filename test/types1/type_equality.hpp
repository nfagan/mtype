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

  bool equivalence_different_types(const types::DestructuredTuple& a, const TypeHandle& b) const;
  bool match_list(const types::List& a, const std::vector<TypeHandle>& b, int64_t ib, int64_t* num_incr_b) const;

  bool equivalence_same_usage(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const;
  bool equivalence_different_usage(const types::DestructuredTuple& a, const types::DestructuredTuple& b) const;

  DebugTypePrinter type_printer() const;

private:
  Type::Tag type_of(const TypeHandle& handle) const;

private:
  const TypeStore& store;
  const StringRegistry& string_registry;
};

}