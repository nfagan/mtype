#include "token.hpp"

namespace mt {

bool Token::is_null() const {
  return type == TokenType::null;
}

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

int64_t TokenIterator::next_index() const {
  return current_index;
}

void TokenIterator::advance() {
  current_index++;
}

void TokenIterator::advance(int64_t num) {
  for (int64_t i = 0; i < num; i++) {
    advance();
  }
}

void TokenIterator::advance_to_one(const TokenType* types, int64_t num_types) {
  bool found_type = false;

  while (has_next() && !found_type) {
    const auto t = peek().type;

    for (int64_t i = 0; i < num_types; i++) {
      if (t == types[i]) {
        found_type = true;
        break;
      }
    }

    if (!found_type) {
      advance();
    }
  }
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

const Token& TokenIterator::peek_prev() const {
  return peek_nth(-1);
}

const Token& TokenIterator::peek_next() const {
  return peek_nth(1);
}

const Token& TokenIterator::null_token() {
  static Token null_token{TokenType::null, std::string_view()};
  return null_token;
}

}
