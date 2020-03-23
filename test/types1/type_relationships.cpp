#include "type_relationships.hpp"
#include "library.hpp"

namespace mt {

/*
 * TypeEquivalence
 */

bool TypeEquivalence::related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const {
  return a.identifier == b.identifier;
}

/*
 * SubtypeRelated
 */

bool SubtypeRelated::related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b, bool rev) const {
  if (rev) {
    return library.subtype_related(rhs, lhs, b, a);
  } else {
    return library.subtype_related(lhs, rhs, a, b);
  }
}

}