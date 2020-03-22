#include "substitution.hpp"

namespace mt {

void Substitution::push_type_equation(TypeEquation&& eq) {
  type_equations.emplace_back(std::move(eq));
}

Optional<TypeHandle> Substitution::bound_type(const TypeHandle& for_type) const {
  const auto lookup = TypeEquationTerm(nullptr, for_type);

  if (bound_variables.count(lookup) == 0) {
    return NullOpt{};
  } else {
    return Optional<TypeHandle>(bound_variables.at(lookup).term);
  }
}

}
