#pragma once

#include "type_relation.hpp"

namespace mt {

class Library;

class TypeEquivalence : public TypeRelationship {
public:
  TypeEquivalence() = default;
  ~TypeEquivalence() override = default;

  bool related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const override;
};

class SubtypeRelated : public TypeRelationship {
public:
  SubtypeRelated(const Library& library) : library(library) {
    //
  }
  ~SubtypeRelated() override = default;

  bool related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const override;

private:
  const Library& library;
};

}