#include "type_relationships.hpp"
#include "library.hpp"

namespace mt {

/*
 * TypeEquivalence
 */

bool EquivalenceRelation::related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const {
  return a.identifier == b.identifier;
}

/*
 * SubtypeRelated
 */

bool SubtypeRelation::related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const {
  if (rev) {
    return library.subtype_related(rhs, lhs, b, a);
  } else {
    return library.subtype_related(lhs, rhs, a, b);
  }
}

}