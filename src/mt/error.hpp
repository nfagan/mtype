#pragma once

#include "token.hpp"
#include <string>
#include <string_view>
#include <memory>

namespace mt {

class TextRowColumnIndices;
class CodeFileDescriptor;
class TokenSourceMap;

class ParseError {
  //  @TODO: Have this class accept ParseSourceData; eliminate TokenSourceMap
  friend class ShowParseErrors;
public:
  ParseError() : descriptor(nullptr) {
    //
  }

  ParseError(std::string_view text, const Token& at_token, std::string message) :
  ParseError(text, at_token, std::move(message), nullptr) {
    //
  }

  ParseError(std::string_view text,
            const Token& at_token,
            std::string message,
            const CodeFileDescriptor* descriptor) :
    text(text), at_token(at_token),
    message(std::move(message)),
    descriptor(descriptor) {
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

  static constexpr int context_amount = 30;
};

using ParseErrors = std::vector<ParseError>;

class ShowParseErrors {
public:
  ShowParseErrors() : is_rich_text(true) {
    //
  }

  void show(const ParseErrors& errs, const TokenSourceMap& source_data);
  void show(const ParseError& err, const TokenSourceMap& source_data, int64_t index = 0);

private:
  const char* stylize(const char* code) const;

public:
  bool is_rich_text;
};

std::string make_error_message_duplicate_type_identifier(std::string_view dup_ident);

}