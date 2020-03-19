#pragma once

#include "token.hpp"
#include <string>
#include <string_view>

namespace mt {

class TextRowColumnIndices;
class CodeFileDescriptor;

class ParseError {
  friend class ShowParseErrors;
public:
  ParseError() : descriptor(nullptr) {
    //
  }

  ParseError(std::string_view text, const Token& at_token, std::string message) : ParseError(text, at_token, message, nullptr) {
    //
  }

  ParseError(std::string_view text, const Token& at_token, std::string message, const CodeFileDescriptor* descriptor) :
    text(text), at_token(at_token), message(std::move(message)), descriptor(descriptor) {
    //
  }

  ~ParseError() = default;

private:
  std::string make_message(bool colorize) const;
  bool is_null_token() const;

private:
  std::string_view text;
  Token at_token;
  std::string message;
  const CodeFileDescriptor* descriptor;
};

using ParseErrors = std::vector<ParseError>;

class ShowParseErrors {
public:
  ShowParseErrors(const TextRowColumnIndices* row_col_indices) :
    row_col_indices(row_col_indices), is_rich_text(true) {
    //
  }

  void show(const ParseErrors& errs);
  void show(const ParseError& err, int64_t index = 0);

private:
  const char* stylize(const char* code) const;

private:
  const TextRowColumnIndices* row_col_indices;

public:
  bool is_rich_text;
};

}