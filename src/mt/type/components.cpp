#include "components.hpp"
#include <functional>

namespace mt {

/*
 * TypeEquationTerm
 */

std::size_t TypeEquationTerm::TypeHash::operator()(const TypeEquationTerm& t) const {
  return std::hash<Type*>{}(t.term);
}

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

TypeReference make_ref(const Token* source_token, Type* term, const TypeScope* scope) {
  return TypeReference(source_token, term, scope);
}

}