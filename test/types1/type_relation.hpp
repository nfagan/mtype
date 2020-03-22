#pragma once

#include "type.hpp"
#include "type_store.hpp"

namespace mt {

class TypeVisitor;
class DebugTypePrinter;

class TypeRelationship {
public:
  TypeRelationship() = default;
  virtual ~TypeRelationship() = default;

  virtual bool related(const types::Scalar& a, const types::Scalar& b, bool rev) const = 0;
};

class TypeRelation {
private:
  struct DestructuredVisitor {
    DestructuredVisitor(const TypeRelation& eq) : eq(eq) {
      //
    }
    bool operator()(const TypeHandle& a, const TypeHandle& b, bool rev) const {
      return eq.related(a, b, rev);
    }

    const TypeRelation& eq;
  };
public:
  struct TypeRelationComparator {
    explicit TypeRelationComparator(const TypeRelation& type_eq) : type_eq(type_eq) {
      //
    }

    bool operator()(const TypeHandle& a, const TypeHandle& b) const;

    const TypeRelation& type_eq;
  };

  struct ArgumentComparator {
    explicit ArgumentComparator(const TypeRelation& type_eq) : type_eq(type_eq) {
      //
    }

    bool operator()(const types::Abstraction& a, const types::Abstraction& b) const;
    const TypeRelation& type_eq;
  };

public:
  TypeRelation(const TypeRelationship& relationship, const TypeStore& store) :
  relationship(relationship), store(store) {
    //
  }
public:
  bool related_entry(const TypeHandle& a, const TypeHandle& b) const;

private:
  using DT = types::DestructuredTuple;

  bool related(const TypeHandle& a, const TypeHandle& b, bool rev) const;

  bool element_wise_related(const TypeHandles& a, const TypeHandles& b, bool rev) const;
  bool related_different_types(const TypeHandle& a, const TypeHandle& b, bool rev) const;
  bool related_same_types(const TypeHandle& a, const TypeHandle& b, bool rev) const;

  bool related(const types::Scalar& a, const types::Scalar& b, bool rev) const;
  bool related(const types::DestructuredTuple& a, const types::DestructuredTuple& b, bool rev) const;
  bool related(const types::List& a, const types::List& b, bool rev) const;
  bool related(const types::Tuple& a, const types::Tuple& b, bool rev) const;
  bool related(const types::Abstraction& a, const types::Abstraction& b, bool rev) const;

  bool related_different_types(const types::DestructuredTuple& a, const TypeHandle& b, bool rev) const;
  bool related_different_types(const types::List& a, const TypeHandle& b, bool rev) const;

  bool related_list(const TypeHandles& a, const TypeHandles& b, int64_t* ia, int64_t* ib,
                    int64_t num_a, int64_t num_b, bool rev) const;
  bool related_list_sub_tuple(const DT& tup_a, const TypeHandles& b, int64_t* ib, int64_t num_b, bool rev) const;

  DebugTypePrinter type_printer() const;

private:
  Type::Tag type_of(const TypeHandle& handle) const;

private:
  const TypeRelationship& relationship;
  const TypeStore& store;
};

}