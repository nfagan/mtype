#pragma once

#include "token.hpp"
#include <string>
#include <string_view>

namespace mt {

class TextRowColumnIndices;

class ParseError {
public:
  ParseError() = default;

  ParseError(std::string_view text, const Token& at_token, std::string message) :
    text(text), at_token(at_token), message(std::move(message)) {
    //
  }

  ~ParseError() = default;

  void show(int64_t index = 0) const;
  void show(const TextRowColumnIndices& row_col_indices, int64_t index = 0) const;

private:
  std::string make_message(bool colorize) const;
  bool is_null_token() const;

private:
  std::string_view text;
  Token at_token;
  std::string message;
};

using ParseErrors = std::vector<ParseError>;
void show_parse_errors(const ParseErrors& errs, const TextRowColumnIndices& row_col_indices);

}