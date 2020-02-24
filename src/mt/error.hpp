#pragma once

#include "token.hpp"
#include <string>
#include <string_view>

namespace mt {

class ParseError {
public:
  ParseError() = default;

  ParseError(std::string_view text, const Token& at_token, std::string message) :
    text(text), at_token(at_token), message(std::move(message)) {
    //
  }

  ~ParseError() = default;

  void show() const;

private:
  std::string_view text;
  Token at_token;
  std::string message;
};

using ParseErrors = std::vector<ParseError>;

void show_parse_errors(const ParseErrors& errs);

}