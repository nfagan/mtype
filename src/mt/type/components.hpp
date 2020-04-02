#pragma once

#include <cstdint>
#include <cstddef>
#include <unordered_map>

namespace mt {

struct Token;
struct Type;

struct TypeIdentifier {
  struct Hash {
    std::size_t operator()(const TypeIdentifier& id) const;
  };

  TypeIdentifier() : TypeIdentifier(-1) {
    //
  }

  explicit TypeIdentifier(int64_t name) : name(name) {
    //
  }

  friend bool operator==(const TypeIdentifier& a, const TypeIdentifier& b) {
    return a.name == b.name;
  }
  friend bool operator!=(const TypeIdentifier& a, const TypeIdentifier& b) {
    return a.name != b.name;
  }

  int64_t name;
};

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

}