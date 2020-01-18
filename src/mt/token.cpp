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
    return std::string(lexeme);
  }
}

}
