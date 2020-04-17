#include "substitution.hpp"

namespace mt {

int64_t Substitution::num_type_equations() const {
  return type_equations.size();
}

int64_t Substitution::num_bound_terms() const {
  return bound_terms.size();
}

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
