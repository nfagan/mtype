#pragma once

#include "ast.hpp"
#include "token.hpp"
#include "Result.hpp"
#include "Optional.hpp"
#include <vector>

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

class AstGenerator {
public:
  AstGenerator() = default;
  ~AstGenerator() = default;

  Result<ParseErrors, BoxedAstNode> parse(const std::vector<Token>& tokens, std::string_view text);

private:
  Result<ParseErrors, BoxedAstNode> block();
  Result<ParseErrors, BoxedAstNode> function_def();
  Result<ParseErrors, FunctionHeader> function_header();
  Result<ParseErrors, std::vector<std::string_view>> function_inputs();
  Result<ParseErrors, std::vector<std::string_view>> function_outputs();
  Result<ParseError, std::string_view> char_identifier();
  Result<ParseErrors, std::vector<std::string_view>> char_identifier_sequence(TokenType terminator);

  ParseError make_error_expected_token_type(const Token& at_token, const TokenType* types, int64_t num_types);
  Optional<ParseError> consume(TokenType type);

private:
  TokenIterator iterator;
  std::string_view text;
};

}