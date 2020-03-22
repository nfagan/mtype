#pragma once

#include "type.hpp"

namespace mt {

class Substitution {
  friend class Unifier;
public:
  using BoundVariables = std::unordered_map<TypeEquationTerm, TypeEquationTerm, TypeEquationTerm::HandleHash>;

  Substitution() = default;

  int64_t num_type_equations() const {
    return type_equations.size();
  }

  int64_t num_bound_variables() const {
    return bound_variables.size();
  }

  void push_type_equation(TypeEquation&& eq);
  Optional<TypeHandle> bound_type(const TypeHandle& for_type) const;

private:
  std::vector<TypeEquation> type_equations;
  BoundVariables bound_variables;
};

}