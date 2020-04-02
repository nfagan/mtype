#pragma once

#include "type.hpp"
#include "type_store.hpp"

namespace mt {

class TypeConstraintGenerator;
class DebugTypePrinter;

class TypeRelationship {
public:
  TypeRelationship() = default;
  virtual ~TypeRelationship() = default;

  virtual bool related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const = 0;
};

class TypeRelation {
  friend class DestructuredVisitor;
public:
  struct ArgumentLess {
    explicit ArgumentLess(const TypeRelation& type_relation) : type_relation(type_relation) {
      //
    }

    bool operator()(const types::Abstraction& a, const types::Abstraction& b) const;
    const TypeRelation& type_relation;
  };

  struct NameLess {
    explicit NameLess(const TypeRelation& type_relation) : type_relation(type_relation) {
      //
    }

    bool operator()(const types::Abstraction& a, const types::Abstraction& b) const;
    const TypeRelation& type_relation;
  };

public:
  TypeRelation(const TypeRelationship& relationship, const TypeStore& store) :
  relationship(relationship), store(store) {
    //
  }
public:
  bool related_entry(const Type* a, const Type* b, bool rev = false) const;

private:
  using DT = types::DestructuredTuple;

  bool related(const Type* a, const Type* b, bool rev) const;

  bool element_wise_related(const TypePtrs& a, const TypePtrs& b, bool rev) const;
  bool related_different_types(const Type* a, const Type* b, bool rev) const;
  bool related_same_types(const Type* a, const Type* b, bool rev) const;

  bool related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const;
  bool related(const Type* lhs, const Type* rhs, const DT& a, const DT& b, bool rev) const;
  bool related(const types::List& a, const types::List& b, bool rev) const;
  bool related(const types::Tuple& a, const types::Tuple& b, bool rev) const;
  bool related(const types::Abstraction& a, const types::Abstraction& b, bool rev) const;
  bool related(const types::Scheme& a, const types::Scheme& b, bool rev) const;

  bool related_different_types(const types::Scheme& a, const Type* b, bool rev) const;
  bool related_different_types(const types::DestructuredTuple& a, const Type* b, bool rev) const;
  bool related_different_types(const types::List& a, const Type* b, bool rev) const;

  bool related_list(const TypePtrs& a, const TypePtrs& b, int64_t* ia, int64_t* ib,
                    int64_t num_a, int64_t num_b, bool rev) const;
  bool related_list_sub_tuple(const DT& tup_a, const TypePtrs& b, int64_t* ib, int64_t num_b, bool rev) const;

  DebugTypePrinter type_printer() const;

private:
  const TypeRelationship& relationship;
  const TypeStore& store;
};

}