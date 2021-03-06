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

  int64_t num_type_equations() const;
  int64_t num_bound_terms() const;

  void push_type_equation(const TypeEquation& eq);
  Optional<Type*> bound_type(const TypeEquationTerm& for_term) const;
  Optional<Type*> bound_type(Type* for_type) const;

private:
  std::vector<TypeEquation> type_equations;
  int64_t equation_index;
  BoundTerms bound_terms;
};

}