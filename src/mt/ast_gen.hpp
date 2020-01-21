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
  parse(const std::vector<Token>& tokens, std::string_view text, bool functions_are_end_terminated);

private:

  Optional<std::unique_ptr<Block>> block();
  Optional<std::unique_ptr<Block>> sub_block();
  Optional<std::unique_ptr<FunctionDef>> function_def();
  Optional<FunctionHeader> function_header();
  Optional<std::vector<std::string_view>> function_inputs();
  Optional<std::vector<std::string_view>> function_outputs();
  Optional<std::string_view> char_identifier();
  Optional<std::vector<std::string_view>> char_identifier_sequence(TokenType terminator);

  Optional<BoxedExpr> expr(bool allow_empty = false);
  Optional<BoxedExpr> anonymous_function_expr(const Token& source_token);
  Optional<BoxedExpr> grouping_expr(const Token& source_token);
  Optional<BoxedExpr> identifier_reference_expr(const Token& source_token);
  Optional<std::unique_ptr<SubscriptExpr>> period_subscript_expr(const Token& source_token);
  Optional<std::unique_ptr<SubscriptExpr>> non_period_subscript_expr(const Token& source_token,
                                                                     SubscriptMethod method,
                                                                     TokenType term);
  Optional<BoxedExpr> literal_field_reference_expr(const Token& source_token);
  Optional<BoxedExpr> dynamic_field_reference_expr(const Token& source_token);
  Optional<BoxedExpr> ignore_output_expr(const Token& source_token);
  Optional<BoxedExpr> literal_expr(const Token& source_token);
  Optional<BoxedExpr> colon_subscript_expr(const Token& source_token);
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
  bool is_ignore_argument_expr(const Token& curr_token) const;
  bool is_colon_subscript_expr(const Token& curr_token) const;

  bool is_command_stmt(const Token& curr_token) const;

  Optional<BoxedStmt> assignment_stmt(BoxedExpr lhs, const Token& initial_token);
  Optional<BoxedStmt> expr_stmt(const Token& source_token);
  Optional<BoxedStmt> command_stmt(const Token& source_token);
  Optional<BoxedStmt> if_stmt(const Token& source_token);
  Optional<BoxedStmt> for_stmt(const Token& source_token);
  Optional<BoxedStmt> while_stmt(const Token& source_token);
  Optional<BoxedStmt> switch_stmt(const Token& source_token);
  Optional<BoxedStmt> control_stmt(const Token& source_token);
  Optional<BoxedStmt> try_stmt(const Token& source_token);
  Optional<BoxedStmt> variable_declaration_stmt(const Token& source_token);
  Optional<BoxedStmt> stmt();

  Optional<IfBranch> if_branch(const Token& source_token);
  Optional<SwitchCase> switch_case(const Token& source_token);

  ParseError make_error_expected_token_type(const Token& at_token, const TokenType* types, int64_t num_types);
  ParseError make_error_reference_after_parens_reference_expr(const Token& at_token);
  ParseError make_error_invalid_expr_token(const Token& at_token);
  ParseError make_error_incomplete_expr(const Token& at_token);
  ParseError make_error_invalid_assignment_target(const Token& at_token);
  ParseError make_error_expected_lhs(const Token& at_token);
  ParseError make_error_semicolon_delimiter_in_parens_grouping_expr(const Token& at_token);
  ParseError make_error_duplicate_otherwise_in_switch_stmt(const Token& at_token);

  Optional<ParseError> consume(TokenType type);
  Optional<ParseError> consume_one_of(const TokenType* types, int64_t num_types);
  void add_error(ParseError&& err);

private:
  TokenIterator iterator;
  std::string_view text;
  bool is_end_terminated_function;

  ParseErrors parse_errors;
};

}