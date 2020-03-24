#pragma once

#include "type.hpp"

namespace mt {

class Substitution {
  friend class Unifier;
public:
  Substitution() = default;

  int64_t num_type_equations() const {
    return type_equations.size();
  }

  int64_t num_bound_terms() const {
    return bound_terms.size();
  }

  void push_type_equation(const TypeEquation& eq);
  void push_type_equation(TypeEquation&& eq);
  Optional<TypeHandle> bound_type(const TypeHandle& for_type) const;

private:
  std::vector<TypeEquation> type_equations;
  BoundTerms bound_terms;
};

}