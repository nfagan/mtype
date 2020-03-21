#pragma once

#include "type.hpp"
#include "type_store.hpp"

namespace mt {

class TypeVisitor;
class DebugTypePrinter;

class TypeEquality {
private:
  struct DestructuredVisitor {
    DestructuredVisitor(const TypeEquality& eq) : eq(eq) {
      //
    }
    bool operator()(const TypeHandle& a, const TypeHandle& b, bool rev) const {
      return eq.equivalence(a, b, rev);
    }

    const TypeEquality& eq;
  };
public:
  struct TypeEquivalenceComparator {
    explicit TypeEquivalenceComparator(const TypeEquality& type_eq) : type_eq(type_eq) {
      //
    }

    bool operator()(const TypeHandle& a, const TypeHandle& b) const;

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
  TypeEquality(const TypeStore& store) : store(store) {
    //
  }
public:
  bool equivalence_entry(const TypeHandle& a, const TypeHandle& b) const;

private:
  using DT = types::DestructuredTuple;

  bool equivalence(const TypeHandle& a, const TypeHandle& b, bool rev) const;

  bool element_wise_equivalence(const TypeHandles& a, const TypeHandles& b, bool rev) const;
  bool equivalence_different_types(const TypeHandle& a, const TypeHandle& b, bool rev) const;
  bool equivalence_same_types(const TypeHandle& a, const TypeHandle& b, bool rev) const;

  bool equivalence(const types::Scalar& a, const types::Scalar& b, bool rev) const;
  bool equivalence(const types::DestructuredTuple& a, const types::DestructuredTuple& b, bool rev) const;
  bool equivalence(const types::List& a, const types::List& b, bool rev) const;
  bool equivalence(const types::Tuple& a, const types::Tuple& b, bool rev) const;
  bool equivalence(const types::Abstraction& a, const types::Abstraction& b, bool rev) const;

  bool equivalence_different_types(const types::DestructuredTuple& a, const TypeHandle& b, bool rev) const;
  bool equivalence_different_types(const types::List& a, const TypeHandle& b, bool rev) const;

  bool equivalence_list(const TypeHandles& a, const TypeHandles& b, int64_t* ia, int64_t* ib,
    int64_t num_a, int64_t num_b, bool rev) const;
  bool equivalence_list_sub_tuple(const DT& tup_a, const TypeHandles& b, int64_t* ib, int64_t num_b, bool rev) const;

  DebugTypePrinter type_printer() const;

private:
  Type::Tag type_of(const TypeHandle& handle) const;

private:
  const TypeStore& store;
};

}