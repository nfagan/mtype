#include "type_relationships.hpp"
#include "library.hpp"

namespace mt {

/*
 * TypeEquivalence
 */

bool EquivalenceRelation::related(const Type* lhs, const Type* rhs, bool) const {
  if (lhs->is_scalar() && rhs->is_scalar()) {
    return MT_SCALAR_REF(*lhs).identifier == MT_SCALAR_REF(*rhs).identifier;
  }

  return false;
}

/*
 * SubtypeRelated
 */

bool SubtypeRelation::related(const Type* lhs, const Type* rhs, bool rev) const {
  if (rev) {
    return library.subtype_related(rhs, lhs);
  } else {
    return library.subtype_related(lhs, rhs);
  }
}

}