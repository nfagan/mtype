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

  Result<ParseError, BoxedExpr> expr(bool allow_empty = false);
  Result<ParseError, BoxedExpr> grouping_expr();
  Result<ParseError, BoxedExpr> identifier_reference_expr();
  Result<ParseError, std::unique_ptr<SubscriptExpr>> period_subscript_expr();
  Result<ParseError, std::unique_ptr<SubscriptExpr>> non_period_subscript_expr(SubscriptMethod method, TokenType term);
  Result<ParseError, BoxedExpr> literal_field_reference_expr();
  Result<ParseError, BoxedExpr> dynamic_field_reference_expr();

  Result<ParseErrors, BoxedAstNode> expr_stmt();

  ParseError make_error_expected_token_type(const Token& at_token, const TokenType* types, int64_t num_types);
  ParseError make_error_reference_after_parens_reference_expr(const Token& at_token);
  ParseError make_error_invalid_expr_token(const Token& at_token);
  ParseError make_error_incomplete_expr(const Token& at_token);

  Optional<ParseError> consume(TokenType type);

private:
  TokenIterator iterator;
  std::string_view text;
};

}