#include "substitution.hpp"

namespace mt {

void Substitution::push_type_equation(const TypeEquation& eq) {
  type_equations.push_back(eq);
}

Optional<Type*> Substitution::bound_type(const TypeEquationTerm& for_type) const {
  if (bound_terms.count(for_type) == 0) {
    return NullOpt{};
  } else {
    return Optional<Type*>(bound_terms.at(for_type).term);
  }
}

}
