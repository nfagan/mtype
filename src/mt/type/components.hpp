#pragma once

#include "../identifier.hpp"
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace mt {

struct Token;
struct Type;
struct TypeScope;

/*
 * TypeReference
 */

struct TypeReference {
  TypeReference() : TypeReference(nullptr, nullptr, nullptr) {
    //
  }

  TypeReference(const Token* source_token, Type* type, const TypeScope* scope) :
  source_token(source_token), type(type), scope(scope) {
    //
  }

  const Token* source_token;
  Type* type;
  const TypeScope* scope;
};

/*
 * TypeEquationTerm
 */

struct TypeEquationTerm {
  struct TypeHash {
    std::size_t operator()(const TypeEquationTerm& t) const;
  };

  struct TypeLess {
    bool operator()(const TypeEquationTerm& a, const TypeEquationTerm& b) const;
  };

  TypeEquationTerm() : source_token(nullptr), term(nullptr) {
    //
  }

  TypeEquationTerm(const Token* source_token, Type* term) :
    source_token(source_token), term(term) {
    //
  }

  friend inline bool operator==(const TypeEquationTerm& a, const TypeEquationTerm& b) {
    return a.term == b.term;
  }

  friend inline bool operator!=(const TypeEquationTerm& a, const TypeEquationTerm& b) {
    return a.term != b.term;
  }

  const Token* source_token;
  Type* term;
};

struct TypeEquation {
  TypeEquation(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs) : lhs(lhs), rhs(rhs) {
    //
  }

  TypeEquationTerm lhs;
  TypeEquationTerm rhs;
};

using TermRef = const TypeEquationTerm&;
using BoundTerms = std::unordered_map<TypeEquationTerm, TypeEquationTerm, TypeEquationTerm::TypeHash>;

TypeEquationTerm make_term(const Token* source_token, Type* term);
TypeEquation make_eq(const TypeEquationTerm& lhs, const TypeEquationTerm& rhs);

MatlabIdentifier to_matlab_identifier(const TypeIdentifier& ident);
TypeIdentifier to_type_identifier(const MatlabIdentifier& ident);

}