#pragma once

#include "ast.hpp"
#include "token.hpp"
#include "Result.hpp"
#include "Optional.hpp"
#include "error.hpp"
#include <vector>

namespace mt {

class AstGenerator {
public:
  AstGenerator() : is_end_terminated_function(true) {
    //
  }

  ~AstGenerator() = default;

  Result<ParseErrors, std::unique_ptr<Block>>
  parse(const std::vector<Token>& tokens, std::string_view text);

private:

  Result<ParseErrors, std::unique_ptr<Block>> block();
  Result<ParseErrors, std::unique_ptr<Block>> sub_block();
  Result<ParseErrors, std::unique_ptr<FunctionDef>> function_def();
  Result<ParseError, FunctionHeader> function_header();
  Result<ParseError, std::vector<std::string_view>> function_inputs();
  Result<ParseError, std::vector<std::string_view>> function_outputs();
  Result<ParseError, std::string_view> char_identifier();
  Result<ParseError, std::vector<std::string_view>> char_identifier_sequence(TokenType terminator);

  Result<ParseError, BoxedExpr> expr(bool allow_empty = false);
  Result<ParseError, BoxedExpr> anonymous_function_expr(const Token& source_token);
  Result<ParseError, BoxedExpr> grouping_expr(const Token& source_token);
  Result<ParseError, BoxedExpr> identifier_reference_expr(const Token& source_token);
  Result<ParseError, std::unique_ptr<SubscriptExpr>> period_subscript_expr(const Token& source_token);
  Result<ParseError, std::unique_ptr<SubscriptExpr>> non_period_subscript_expr(const Token& source_token,
                                                                               SubscriptMethod method,
                                                                               TokenType term);
  Result<ParseError, BoxedExpr> literal_field_reference_expr(const Token& source_token);
  Result<ParseError, BoxedExpr> dynamic_field_reference_expr(const Token& source_token);
  Result<ParseError, BoxedExpr> ignore_output_expr(const Token& source_token);
  Result<ParseError, BoxedExpr> literal_expr(const Token& source_token);
  Result<ParseError, BoxedExpr> colon_subscript_expr(const Token& source_token);
  std::unique_ptr<UnaryOperatorExpr> pending_unary_prefix_expr(const Token& source_token);
  Optional<ParseError> pending_binary_expr(const Token& source_token,
                                           std::vector<BoxedExpr>& completed,
                                           std::vector<BoxedBinaryOperatorExpr>& binaries);
  Optional<ParseError> postfix_unary_expr(const Token& source_token,
                                          std::vector<BoxedExpr>& completed);
  Optional<ParseError> handle_postfix_unary_exprs(std::vector<BoxedExpr>& completed);
  void handle_prefix_unary_exprs(std::vector<BoxedExpr>& completed,
                                 std::vector<BoxedUnaryOperatorExpr>& unaries);
  void handle_binary_exprs(std::vector<BoxedExpr>& completed,
                           std::vector<BoxedBinaryOperatorExpr>& binaries,
                           std::vector<BoxedBinaryOperatorExpr>& pending_binaries);

  bool is_unary_prefix_expr(const Token& curr_token) const;
  bool is_ignore_output_expr(const Token& curr_token) const;
  bool is_colon_subscript_expr(const Token& curr_token) const;

  Result<ParseError, BoxedStmt> assignment_stmt(BoxedExpr lhs, const Token& initial_token);
  Result<ParseError, BoxedStmt> expr_stmt();
  Result<ParseErrors, BoxedStmt> if_stmt();
  Result<ParseErrors, BoxedStmt> for_stmt();
  Result<ParseErrors, BoxedAstNode> stmt();

  Result<ParseErrors, std::unique_ptr<IfBranch>> if_branch(const Token& source_token);

  ParseError make_error_expected_token_type(const Token& at_token, const TokenType* types, int64_t num_types);
  ParseError make_error_reference_after_parens_reference_expr(const Token& at_token);
  ParseError make_error_invalid_expr_token(const Token& at_token);
  ParseError make_error_incomplete_expr(const Token& at_token);
  ParseError make_error_invalid_assignment_target(const Token& at_token);
  ParseError make_error_expected_lhs(const Token& at_token);

  Optional<ParseError> consume(TokenType type);

private:
  TokenIterator iterator;
  std::string_view text;
  bool is_end_terminated_function;
};

}