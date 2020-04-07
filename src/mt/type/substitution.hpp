#pragma once

#include "types.hpp"
#include "../Optional.hpp"
#include <vector>

namespace mt {

class Substitution {
  friend class Unifier;
public:
  Substitution() : equation_index(0) {
    //
  }

  int64_t num_type_equations() const {
    return type_equations.size();
  }

  int64_t num_bound_terms() const {
    return bound_terms.size();
  }

  void push_type_equation(const TypeEquation& eq);
  Optional<Type*> bound_type(const TypeEquationTerm& for_type) const;

private:
  std::vector<TypeEquation> type_equations;
  int64_t equation_index;
  BoundTerms bound_terms;
};

}