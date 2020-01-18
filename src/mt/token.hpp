#pragma once

#include "token_type.hpp"
#include <string_view>
#include <string>
#include <iostream>

namespace mt {

struct Token {
  TokenType type;
  std::string_view lexeme;

  std::string pretty_lexeme() const;
};

}

inline std::ostream& operator<<(std::ostream& stream, const mt::Token& tok) {
  stream << to_string(tok.type) << ": " << tok.pretty_lexeme();
  return stream;
}