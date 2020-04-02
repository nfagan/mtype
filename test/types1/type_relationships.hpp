#pragma once

#include "type_relation.hpp"

namespace mt {

class Library;

class EquivalenceRelation : public TypeRelationship {
public:
  EquivalenceRelation() = default;
  ~EquivalenceRelation() override = default;

  bool related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const override;
};

class SubtypeRelation : public TypeRelationship {
public:
  SubtypeRelation(const Library& library) : library(library) {
    //
  }
  ~SubtypeRelation() override = default;

  bool related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const override;

private:
  const Library& library;
};

}