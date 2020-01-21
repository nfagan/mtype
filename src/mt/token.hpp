#pragma once

#include "token_type.hpp"
#include <string_view>
#include <string>
#include <iostream>
#include <vector>

namespace mt {

struct Token {
  TokenType type;
  std::string_view lexeme;

  std::string pretty_lexeme() const;
};

class TokenIterator {
public:
  TokenIterator() : TokenIterator(nullptr) {
    //
  }
  explicit TokenIterator(const std::vector<Token>* tokens) : tokens(tokens), current_index(0) {
    //
  }
  ~TokenIterator() = default;

  bool has_next() const;
  int64_t next_index() const;

  const Token& peek() const;
  const Token& peek_nth(int64_t num) const;
  const Token& peek_prev() const;
  const Token& peek_next() const;

  void advance();
  void advance(int64_t num);
  void advance_to_one(const TokenType* types, int64_t num_types);

private:
  static const Token& null_token();

private:
  const std::vector<Token>* tokens;
  int64_t current_index;
};

}

inline std::ostream& operator<<(std::ostream& stream, const mt::Token& tok) {
  stream << to_string(tok.type) << ": " << tok.pretty_lexeme();
  return stream;
}