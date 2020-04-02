#include "components.hpp"
#include <functional>

namespace mt {

/*
 * TypeIdentifier
 */

std::size_t TypeIdentifier::Hash::operator()(const TypeIdentifier& id) const {
  return std::hash<int64_t>{}(id.name);
}

/*
 * TypeEquationTerm
 */

std::size_t TypeEquationTerm::TypeHash::operator()(const TypeEquationTerm& t) const {
  return std::hash<Type*>{}(t.term);
};

bool TypeEquationTerm::TypeLess::operator()(const TypeEquationTerm& a, const TypeEquationTerm& b) const {
  return a.term < b.term;
}

/*
 * Util
 */

TypeEquationTerm make_term(const Token* source_token, Type* term) {
  return TypeEquationTerm(source_token, term);
}

TypeEquation make_eq(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs) {
  return TypeEquation(lhs, rhs);
}


}