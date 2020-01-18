#include "token.hpp"

namespace mt {

std::string Token::pretty_lexeme() const {
  if (type == TokenType::null) {
    return "<null>";

  } else if (type == TokenType::char_literal) {
    return std::string("'") + std::string(lexeme) + "'";

  } else if (type == TokenType::string_literal) {
    return std::string("\"") + std::string(lexeme) + "\"";

  } else {
    if (lexeme == "\n") {
      return "<new_line>";
    } else {
      return std::string(lexeme);
    }
  }
}

bool TokenIterator::has_next() const {
  return tokens ? current_index < int64_t(tokens->size()) : false;
}

void TokenIterator::advance() {
  current_index++;
}

const Token& TokenIterator::peek() const {
  if (has_next()) {
    return (*tokens)[current_index];
  } else {
    return null_token();
  }
}

const Token& TokenIterator::peek_nth(int64_t num) const {
  if (!tokens) {
    return null_token();
  }

  auto ind = num + current_index;

  if (ind < 0 || ind > int64_t(tokens->size())) {
    return null_token();
  } else {
    return (*tokens)[ind];
  }
}

const Token& TokenIterator::null_token() {
  static Token null_token{TokenType::null, std::string_view()};
  return null_token;
}

}
