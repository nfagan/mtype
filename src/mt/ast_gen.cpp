#include "ast_gen.hpp"
#include "string.hpp"
#include "character.hpp"
#include <array>
#include <cassert>

namespace mt {

Result<ParseErrors, std::unique_ptr<Block>>
AstGenerator::parse(const std::vector<Token>& tokens, std::string_view txt) {
  iterator = TokenIterator(&tokens);
  text = txt;
  is_end_terminated_function = true;
  parse_errors.clear();

  auto result = block();
  if (result) {
    return make_success<ParseErrors, std::unique_ptr<Block>>(result.rvalue());
  } else {
    return make_error<ParseErrors, std::unique_ptr<Block>>(std::move(parse_errors));
  }
}

Optional<std::string_view> AstGenerator::char_identifier() {
  auto tok = iterator.peek();
  auto err = consume(TokenType::identifier);

  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  } else {
    return Optional<std::string_view>(tok.lexeme);
  }
}

Optional<std::vector<std::string_view>> AstGenerator::char_identifier_sequence(TokenType terminator) {
  std::vector<std::string_view> identifiers;
  bool expect_identifier = true;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    const auto& tok = iterator.peek();

    if (expect_identifier && tok.type != TokenType::identifier) {
      std::array<TokenType, 2> possible_types{{TokenType::identifier, terminator}};
      auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
      add_error(std::move(err));
      return NullOpt{};

    } else if (expect_identifier && tok.type == TokenType::identifier) {
      identifiers.push_back(tok.lexeme);
      expect_identifier = false;

    } else if (!expect_identifier && tok.type == TokenType::comma) {
      expect_identifier = true;

    } else {
      std::array<TokenType, 2> possible_types{{TokenType::comma, terminator}};
      auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
      add_error(std::move(err));
      return NullOpt{};
    }

    iterator.advance();
  }

  auto term_err = consume(terminator);
  if (term_err) {
    add_error(term_err.rvalue());
    return NullOpt{};
  } else {
    return Optional<std::vector<std::string_view>>(std::move(identifiers));
  }
}

Optional<FunctionHeader> AstGenerator::function_header() {
  auto output_res = function_outputs();
  if (!output_res) {
    return NullOpt{};
  }

  if (!output_res.value().empty()) {
    auto err = consume(TokenType::equal);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }
  }

  const auto& name_token = iterator.peek();
  auto name_res = char_identifier();
  if (!name_res) {
    return NullOpt{};
  }

  auto input_res = function_inputs();
  if (!input_res) {
    return NullOpt{};
  }

  FunctionHeader header{name_token, name_res.rvalue(), output_res.rvalue(), input_res.rvalue()};
  return Optional<FunctionHeader>(std::move(header));
}

Optional<std::vector<std::string_view>> AstGenerator::function_inputs() {
  if (iterator.peek().type == TokenType::left_parens) {
    iterator.advance();
    return char_identifier_sequence(TokenType::right_parens);
  } else {
    return Optional<std::vector<std::string_view>>(std::vector<std::string_view>());
  }
}

Optional<std::vector<std::string_view>> AstGenerator::function_outputs() {
  const auto& tok = iterator.peek();
  std::vector<std::string_view> outputs;

  if (tok.type == TokenType::left_bracket) {
    iterator.advance();
    return char_identifier_sequence(TokenType::right_bracket);

  } else if (tok.type == TokenType::identifier) {
    const auto& next_tok = iterator.peek_nth(1);
    const bool is_single_output = next_tok.type == TokenType::equal;

    if (is_single_output) {
      outputs.push_back(tok.lexeme);
      iterator.advance();
    }

    return Optional<std::vector<std::string_view>>(std::move(outputs));
  } else {
    std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
    auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
    add_error(std::move(err));
    return NullOpt{};
  }
}

Optional<std::unique_ptr<FunctionDef>> AstGenerator::function_def() {
  iterator.advance();
  auto header_result = function_header();

  if (!header_result) {
    return NullOpt{};
  }

  auto body_res = block();
  if (!body_res) {
    return NullOpt{};
  }

  if (is_end_terminated_function) {
    auto err = consume(TokenType::keyword_end);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }
  }

  auto success = std::make_unique<FunctionDef>(header_result.rvalue(), body_res.rvalue());
  return Optional<std::unique_ptr<FunctionDef>>(std::move(success));
}

Optional<std::unique_ptr<Block>> AstGenerator::block() {
  auto block_node = std::make_unique<Block>();
  bool should_proceed = true;
  bool any_error = false;

  while (iterator.has_next() && should_proceed) {
    BoxedAstNode node;
    bool success = true;

    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::new_line:
      case TokenType::null:
      case TokenType::semicolon:
      case TokenType::comma:
        iterator.advance();
        break;
      case TokenType::keyword_end:
        should_proceed = false;
        break;
      case TokenType::keyword_function: {
        auto def_res = function_def();
        if (def_res) {
          node = def_res.rvalue();
        } else {
          success = false;
        }
        break;
      }
      default: {
        auto stmt_res = stmt();
        if (stmt_res) {
          node = stmt_res.rvalue();
        } else {
          success = false;
        }
      }
    }

    if (!success) {
      any_error = true;
      std::array<TokenType, 1> possible_types{{TokenType::keyword_function}};
      iterator.advance_to_one(possible_types.data(), possible_types.size());

    } else if (node) {
      block_node->append(std::move(node));
    }
  }

  if (any_error) {
    return NullOpt{};
  } else {
    return Optional<std::unique_ptr<Block>>(std::move(block_node));
  }
}

Optional<std::unique_ptr<Block>> AstGenerator::sub_block() {
  auto block_node = std::make_unique<Block>();
  bool should_proceed = true;
  bool any_error = false;

  while (iterator.has_next() && should_proceed) {
    BoxedAstNode node;
    bool success = true;

    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::new_line:
      case TokenType::null:
      case TokenType::semicolon:
      case TokenType::comma:
        iterator.advance();
        break;
      case TokenType::keyword_end:
      case TokenType::keyword_else:
      case TokenType::keyword_elseif:
      case TokenType::keyword_catch:
      case TokenType::keyword_case:
      case TokenType::keyword_otherwise:
        should_proceed = false;
        break;
      default: {
        auto stmt_res = stmt();
        if (stmt_res) {
          node = stmt_res.rvalue();
        } else {
          success = false;
        }
      }
    }

    if (!success) {
      any_error = true;
      std::array<TokenType, 2> possible_types{
        {TokenType::keyword_if, TokenType::keyword_for}
      };
      iterator.advance_to_one(possible_types.data(), possible_types.size());

    } else if (node) {
      block_node->append(std::move(node));
    }
  }

  if (any_error) {
    return NullOpt{};
  } else {
    return Optional<std::unique_ptr<Block>>(std::move(block_node));
  }
}

Optional<BoxedExpr> AstGenerator::literal_field_reference_expr(const Token& source_token) {
  auto ident_res = char_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  auto expr = std::make_unique<LiteralFieldReferenceExpr>(source_token);
  return Optional<BoxedExpr>(std::move(expr));
}

Optional<BoxedExpr> AstGenerator::dynamic_field_reference_expr(const Token& source_token) {
  iterator.advance(); //  consume (

  auto expr_res = expr();
  if (!expr_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::right_parens);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto expr = std::make_unique<DynamicFieldReferenceExpr>(source_token, expr_res.rvalue());
  return Optional<BoxedExpr>(std::move(expr));
}

Optional<std::unique_ptr<SubscriptExpr>>
AstGenerator::non_period_subscript_expr(const Token& source_token,
                                        mt::SubscriptMethod method,
                                        mt::TokenType term) {
  //  @TODO: Recode `end` to `end subscript reference`.
  iterator.advance(); //  consume '(' or '{'
  std::vector<BoxedExpr> args;

  while (iterator.has_next() && iterator.peek().type != term) {
    auto expr_res = expr();
    if (!expr_res) {
      return NullOpt{};
    }

    args.emplace_back(expr_res.rvalue());

    if (iterator.peek().type == TokenType::comma) {
      iterator.advance();
    } else {
      break;
    }
  }

  auto err = consume(term);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto sub = std::make_unique<SubscriptExpr>(source_token, method, std::move(args));
  return Optional<std::unique_ptr<SubscriptExpr>>(std::move(sub));
}

Optional<std::unique_ptr<SubscriptExpr>> AstGenerator::period_subscript_expr(const Token& source_token) {
  iterator.advance(); //  consume '.'

  const auto& tok = iterator.peek();
  Optional<BoxedExpr> expr_res;

  switch (tok.type) {
    case TokenType::identifier:
      expr_res = literal_field_reference_expr(tok);
      break;
    case TokenType::left_parens:
      expr_res = dynamic_field_reference_expr(tok);
      break;
    default:
      std::array<TokenType, 2> possible_types{{TokenType::identifier, TokenType::left_parens}};
      auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
      add_error(std::move(err));
      return NullOpt{};
  }

  if (!expr_res) {
    return NullOpt{};
  }

  std::vector<BoxedExpr> args;
  args.emplace_back(expr_res.rvalue());
  auto subscript_expr = std::make_unique<SubscriptExpr>(source_token, SubscriptMethod::period, std::move(args));

  return Optional<std::unique_ptr<SubscriptExpr>>(std::move(subscript_expr));
}

Optional<BoxedExpr> AstGenerator::identifier_reference_expr(const Token& source_token) {
  auto main_ident_res = char_identifier();
  if (!main_ident_res) {
    return NullOpt{};
  }

  std::vector<std::unique_ptr<SubscriptExpr>> subscripts;
  SubscriptMethod prev_method = SubscriptMethod::unknown;

  while (iterator.has_next()) {
    const auto& tok = iterator.peek();
    //  @Note: Fill with nullptr so that sub_result != NullOpt{}
    Optional<std::unique_ptr<SubscriptExpr>> sub_result(nullptr);
    bool has_subscript = true;

    if (tok.type == TokenType::period) {
      sub_result = period_subscript_expr(tok);

    } else if (tok.type == TokenType::left_parens) {
      sub_result = non_period_subscript_expr(tok, SubscriptMethod::parens, TokenType::right_parens);

    } else if (tok.type == TokenType::left_brace) {
      sub_result = non_period_subscript_expr(tok, SubscriptMethod::brace, TokenType::right_brace);

    } else {
      has_subscript = false;
    }

    if (!sub_result) {
      return NullOpt{};

    } else if (!has_subscript) {
      break;
    }

    auto sub_expr = sub_result.rvalue();

    if (prev_method == SubscriptMethod::parens && sub_expr->method != SubscriptMethod::period) {
      add_error(make_error_reference_after_parens_reference_expr(tok));
      return NullOpt{};
    } else {
      prev_method = sub_expr->method;
      subscripts.emplace_back(std::move(sub_expr));
    }
  }

  auto node = std::make_unique<IdentifierReferenceExpr>(source_token, std::move(subscripts));
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::anonymous_function_expr(const mt::Token& source_token) {
  iterator.advance(); //  consume '@'
  const auto& next_token = iterator.peek();

  if (next_token.type == TokenType::identifier) {
    //  @main
    auto res = identifier_reference_expr(next_token);
    if (!res) {
      return NullOpt{};
    }

    auto node = std::make_unique<FunctionReferenceExpr>(source_token, res.rvalue());
    return Optional<BoxedExpr>(std::move(node));
  }

  //  Expect @(x, y, z) expr
  auto err = consume(TokenType::left_parens);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto input_res = char_identifier_sequence(TokenType::right_parens);
  if (!input_res) {
    return NullOpt{};
  }

  auto body_res = expr();
  if (!body_res) {
    return NullOpt{};
  }

  auto node = std::make_unique<AnonymousFunctionExpr>(source_token,
    input_res.rvalue(), body_res.rvalue());

  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::grouping_expr(const Token& source_token) {
  const TokenType terminator = grouping_terminator_for(source_token.type);
  iterator.advance();

  const bool allow_empty = source_token.type == TokenType::left_bracket ||
    source_token.type == TokenType::left_brace;
  std::vector<std::unique_ptr<GroupingExprComponent>> exprs;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    auto expr_res = expr(allow_empty);
    if (!expr_res) {
      return NullOpt{};
    }

    const auto& next = iterator.peek();
    TokenType delim = TokenType::comma;

    if (next.type == TokenType::comma) {
      iterator.advance();

    } else if (next.type == TokenType::semicolon || next.type == TokenType::new_line) {
      iterator.advance();

      if (source_token.type == TokenType::left_parens) {
        //  (1; 2)  is illegal.
        add_error(make_error_semicolon_delimiter_in_parens_grouping_expr(next));
        return NullOpt{};
      } else {
        delim = TokenType::semicolon;
      }

    } else if (next.type != terminator) {
      std::array<TokenType, 4> possible_types{
        {TokenType::comma, TokenType::semicolon, TokenType::new_line, terminator}
      };
      auto err = make_error_expected_token_type(next, possible_types.data(), possible_types.size());
      add_error(std::move(err));
      return NullOpt{};
    }

    if (expr_res.value()) {
      //  If non-empty.
      auto component_expr = std::make_unique<GroupingExprComponent>(expr_res.rvalue(), delim);
      exprs.emplace_back(std::move(component_expr));
    }
  }

  auto err = consume(terminator);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  const auto grouping_method = grouping_method_from_token_type(source_token.type);
  auto expr_node = std::make_unique<GroupingExpr>(source_token, grouping_method, std::move(exprs));
  return Optional<BoxedExpr>(std::move(expr_node));
}

Optional<BoxedExpr> AstGenerator::colon_subscript_expr(const mt::Token& source_token) {
  iterator.advance(); //  consume ':'

  auto node = std::make_unique<ColonSubscriptExpr>(source_token);
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::literal_expr(const Token& source_token) {
  iterator.advance();
  BoxedExpr expr_node;

  if (source_token.type == TokenType::number_literal) {
    const double num = std::stod(source_token.lexeme.data());
    expr_node = std::make_unique<NumberLiteralExpr>(source_token, num);

  } else if (source_token.type == TokenType::char_literal) {
    expr_node = std::make_unique<CharLiteralExpr>(source_token);

  } else if (source_token.type == TokenType::string_literal) {
    expr_node = std::make_unique<StringLiteralExpr>(source_token);

  } else {
    assert(false && "Unhandled literal expression.");
  }

  return Optional<BoxedExpr>(std::move(expr_node));
}

Optional<BoxedExpr> AstGenerator::ignore_output_expr(const Token& source_token) {
  iterator.advance();

  auto source_node = std::make_unique<IgnoreFunctionOutputArgumentExpr>(source_token);
  return Optional<BoxedExpr>(std::move(source_node));
}

std::unique_ptr<UnaryOperatorExpr> AstGenerator::pending_unary_prefix_expr(const mt::Token& source_token) {
  iterator.advance();
  const auto op = unary_operator_from_token_type(source_token.type);
  //  Awaiting
  return std::make_unique<UnaryOperatorExpr>(source_token, op, nullptr);
}

Optional<ParseError> AstGenerator::postfix_unary_expr(const mt::Token& source_token,
                                                      std::vector<BoxedExpr>& completed) {
  iterator.advance();

  if (completed.empty()) {
    return Optional<ParseError>(make_error_expected_lhs(source_token));
  }

  const auto op = unary_operator_from_token_type(source_token.type);
  auto node = std::make_unique<UnaryOperatorExpr>(source_token, op, std::move(completed.back()));
  completed.back() = std::move(node);

  return NullOpt{};
}

Optional<ParseError>
AstGenerator::handle_postfix_unary_exprs(std::vector<BoxedExpr>& completed) {
  while (iterator.has_next()) {
    const auto& curr = iterator.peek();
    const auto& prev = iterator.peek_prev();

    if (represents_postfix_unary_operator(curr.type) &&
        can_precede_postfix_unary_operator(prev.type)) {
      auto err = postfix_unary_expr(curr, completed);
      if (err) {
        return err;
      }
    } else {
      break;
    }
  }

  return NullOpt{};
}

void AstGenerator::handle_prefix_unary_exprs(std::vector<BoxedExpr>& completed,
                                             std::vector<BoxedUnaryOperatorExpr>& unaries) {
  while (!unaries.empty()) {
    auto un = std::move(unaries.back());
    unaries.pop_back();
    assert(!un->expr && "Unary already had an expression.");
    un->expr = std::move(completed.back());
    completed.back() = std::move(un);
  }
}

void AstGenerator::handle_binary_exprs(std::vector<BoxedExpr>& completed,
                                       std::vector<BoxedBinaryOperatorExpr>& binaries,
                                       std::vector<BoxedBinaryOperatorExpr>& pending_binaries) {
  assert(!binaries.empty() && "Binaries were empty.");
  auto bin = std::move(binaries.back());
  binaries.pop_back();

  auto prec_curr = precedence(bin->op);
  const auto op_next = binary_operator_from_token_type(iterator.peek().type);
  const auto prec_next = precedence(op_next);

  if (prec_curr < prec_next) {
    pending_binaries.emplace_back(std::move(bin));
  } else {
    assert(!bin->right && "Binary already had an expression.");
    auto complete = std::move(completed.back());
    bin->right = std::move(complete);
    completed.back() = std::move(bin);

    while (!pending_binaries.empty() && prec_next < prec_curr) {
      auto pend = std::move(pending_binaries.back());
      pending_binaries.pop_back();
      prec_curr = precedence(pend->op);
      pend->right = std::move(completed.back());
      completed.back() = std::move(pend);
    }
  }
}

Optional<ParseError> AstGenerator::pending_binary_expr(const mt::Token& source_token,
                                                       std::vector<BoxedExpr>& completed,
                                                       std::vector<BoxedBinaryOperatorExpr>& binaries) {
  iterator.advance(); //  Consume operator token.

  if (completed.empty()) {
    return Optional<ParseError>(make_error_expected_lhs(source_token));
  }

  auto left = std::move(completed.back());
  completed.pop_back();

  const auto op = binary_operator_from_token_type(source_token.type);
  auto pending = std::make_unique<BinaryOperatorExpr>(source_token, op, std::move(left), nullptr);
  binaries.emplace_back(std::move(pending));

  return NullOpt{};
}

Optional<BoxedExpr> AstGenerator::expr(bool allow_empty) {
  std::vector<BoxedBinaryOperatorExpr> pending_binaries;
  std::vector<BoxedBinaryOperatorExpr> binaries;
  std::vector<BoxedUnaryOperatorExpr> unaries;
  std::vector<BoxedExpr> completed;

  while (iterator.has_next()) {
    Optional<BoxedExpr> node_res(nullptr);
    const auto& tok = iterator.peek();

    if (tok.type == TokenType::identifier) {
      node_res = identifier_reference_expr(tok);

    } else if (is_ignore_output_expr(tok)) {
      node_res = ignore_output_expr(tok);

    } else if (represents_grouping_initiator(tok.type)) {
      node_res = grouping_expr(tok);

    } else if (represents_literal(tok.type)) {
      node_res = literal_expr(tok);

    } else if (tok.type == TokenType::op_end) {
      iterator.advance();
      node_res = Optional<BoxedExpr>(std::make_unique<EndOperatorExpr>(tok));

    } else if (is_unary_prefix_expr(tok)) {
      unaries.emplace_back(pending_unary_prefix_expr(tok));

    } else if (represents_binary_operator(tok.type)) {
      if (is_colon_subscript_expr(tok)) {
        node_res = colon_subscript_expr(tok);
      } else {
        auto bin_res = pending_binary_expr(tok, completed, binaries);
        if (bin_res) {
          add_error(bin_res.rvalue());
          return NullOpt{};
        }
      }

    } else if (tok.type == TokenType::at) {
      node_res = anonymous_function_expr(tok);

    } else if (represents_expr_terminator(tok.type)) {
      break;

    } else {
      add_error(make_error_invalid_expr_token(tok));
      return NullOpt{};
    }

    if (!node_res) {
      //  An error occurred in one of the sub-expressions.
      return NullOpt{};
    }

    if (node_res.value()) {
      completed.emplace_back(node_res.rvalue());

      auto err = handle_postfix_unary_exprs(completed);
      if (err) {
        add_error(err.rvalue());
        return NullOpt{};
      }
    }

    if (!completed.empty()) {
      handle_prefix_unary_exprs(completed, unaries);
    }

    if (!completed.empty() && !binaries.empty()) {
      handle_binary_exprs(completed, binaries, pending_binaries);
    }
  }

  if (completed.size() == 1) {
    return Optional<BoxedExpr>(std::move(completed.back()));

  } else {
    const bool is_complete = completed.empty() && pending_binaries.empty() && binaries.empty() && unaries.empty();
    const bool is_valid = is_complete && allow_empty;

    if (!is_valid) {
      add_error(make_error_incomplete_expr(iterator.peek()));
      return NullOpt{};
    } else {
      return Optional<BoxedExpr>(nullptr);
    }
  }
}

Optional<BoxedStmt> AstGenerator::control_stmt(const Token& source_token) {
  iterator.advance();

  const auto kind = control_flow_manipulator_from_token_type(source_token.type);
  auto node = std::make_unique<ControlStmt>(source_token, kind);

  return Optional<BoxedStmt>(std::move(node));
}

Optional<SwitchCase> AstGenerator::switch_case(const mt::Token& source_token) {
  iterator.advance();

  auto condition_expr_res = expr();
  if (!condition_expr_res) {
    return NullOpt{};
  }

  auto block_res = sub_block();
  if (!block_res) {
    return NullOpt{};
  }

  SwitchCase switch_case(source_token, condition_expr_res.rvalue(), block_res.rvalue());
  return Optional<SwitchCase>(std::move(switch_case));
}

Optional<BoxedStmt> AstGenerator::switch_stmt(const mt::Token& source_token) {
  iterator.advance();

  auto condition_expr_res = expr();
  if (!condition_expr_res) {
    return NullOpt{};
  }

  std::vector<SwitchCase> cases;
  BoxedBlock otherwise;

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::new_line:
      case TokenType::comma:
        iterator.advance();
        break;
      case TokenType::keyword_case: {
        auto case_res = switch_case(tok);
        if (case_res) {
          cases.emplace_back(case_res.rvalue());
        } else {
          return NullOpt{};
        }
        break;
      }
      case TokenType::keyword_otherwise: {
        iterator.advance();

        if (otherwise) {
          //  Duplicate otherwise in switch statement.
          add_error(make_error_duplicate_otherwise_in_switch_stmt(tok));
          return NullOpt{};
        }

        auto block_res = sub_block();
        if (!block_res) {
          return NullOpt{};
        } else {
          otherwise = block_res.rvalue();
        }
        break;
      }
      default: {
        std::array<TokenType, 2> possible_types{
          {TokenType::keyword_case, TokenType::keyword_otherwise}
        };
        auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
        add_error(std::move(err));
        return NullOpt{};
      }
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<SwitchStmt>(source_token,
    condition_expr_res.rvalue(), std::move(cases), std::move(otherwise));

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::while_stmt(const Token& source_token) {
  iterator.advance();

  auto loop_condition_expr_res = expr();
  if (!loop_condition_expr_res) {
    return NullOpt{};
  }

  auto body_res = sub_block();
  if (!body_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<WhileStmt>(source_token,
    loop_condition_expr_res.rvalue(), body_res.rvalue());

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::for_stmt(const Token& source_token) {
  iterator.advance();

  auto loop_var_res = char_identifier();
  if (!loop_var_res) {
    return NullOpt{};
  }

  auto eq_err = consume(TokenType::equal);
  if (eq_err) {
    add_error(eq_err.rvalue());
    return NullOpt{};
  }

  auto initializer_res = expr();
  if (!initializer_res) {
    return NullOpt{};
  }

  auto block_res = sub_block();
  if (!block_res) {
    return NullOpt{};
  }

  auto end_err = consume(TokenType::keyword_end);
  if (end_err) {
    add_error(end_err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<ForStmt>(source_token, loop_var_res.rvalue(),
    initializer_res.rvalue(), block_res.rvalue());

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::if_stmt(const Token& source_token) {
  auto main_branch_res = if_branch(source_token);
  if (!main_branch_res) {
    return NullOpt{};
  }

  std::vector<IfBranch> elseif_branches;

  while (iterator.peek().type == TokenType::keyword_elseif) {
    auto sub_branch_res = if_branch(iterator.peek());
    if (!sub_branch_res) {
      return NullOpt{};
    } else {
      elseif_branches.emplace_back(sub_branch_res.rvalue());
    }
  }

  Optional<ElseBranch> else_branch = NullOpt{};
  const auto& maybe_else_token = iterator.peek();

  if (maybe_else_token.type == TokenType::keyword_else) {
    iterator.advance();

    auto else_block_res = sub_block();
    if (!else_block_res) {
      return NullOpt{};
    } else {
      else_branch = ElseBranch(maybe_else_token, else_block_res.rvalue());
    }
  }

  auto err = consume(TokenType::keyword_end);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto if_stmt_node = std::make_unique<IfStmt>(main_branch_res.rvalue(),
    std::move(elseif_branches), std::move(else_branch));

  return Optional<BoxedStmt>(std::move(if_stmt_node));
}

Optional<IfBranch> AstGenerator::if_branch(const Token& source_token) {
  iterator.advance(); //  consume if

  auto condition_res = expr();
  if (!condition_res) {
    return NullOpt{};
  }

  auto block_res = sub_block();
  if (!block_res) {
    return NullOpt{};
  }

  IfBranch if_branch(source_token, condition_res.rvalue(), block_res.rvalue());
  return Optional<IfBranch>(std::move(if_branch));
}

Optional<BoxedStmt> AstGenerator::assignment_stmt(BoxedExpr lhs, const Token& initial_token) {
  iterator.advance(); //  consume =

  if (!lhs->is_valid_assignment_target()) {
    add_error(make_error_invalid_assignment_target(initial_token));
    return NullOpt{};
  }

  auto rhs_res = expr();
  if (!rhs_res) {
    return NullOpt{};
  }

  auto assign_stmt = std::make_unique<AssignmentStmt>(std::move(lhs), rhs_res.rvalue());
  return Optional<BoxedStmt>(std::move(assign_stmt));
}

Optional<BoxedStmt> AstGenerator::command_stmt(const Token& source_token) {
  auto ident_res = char_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  bool should_proceed = true;
  std::vector<CharLiteralExpr> arguments;

  while (iterator.has_next() && should_proceed) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::null:
      case TokenType::new_line:
      case TokenType::semicolon:
      case TokenType::comma:
        should_proceed = false;
        break;
      default: {
        arguments.emplace_back(CharLiteralExpr(tok));
        iterator.advance();
      }
    }
  }

  std::array<TokenType, 4> possible_types{{
    TokenType::null, TokenType::new_line, TokenType::semicolon, TokenType::comma
  }};

  auto err = consume_one_of(possible_types.data(), possible_types.size());
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<CommandStmt>(source_token, std::move(arguments));
  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::expr_stmt(const Token& source_token) {
  auto expr_res = expr();

  if (!expr_res) {
    return NullOpt{};
  }

  const auto& curr_token = iterator.peek();

  if (curr_token.type == TokenType::equal) {
    return assignment_stmt(expr_res.rvalue(), source_token);
  }

  auto expr_stmt = std::make_unique<ExprStmt>(expr_res.rvalue());
  return Optional<BoxedStmt>(std::move(expr_stmt));
}

Optional<BoxedStmt> AstGenerator::try_stmt(const mt::Token& source_token) {
  iterator.advance();

  auto try_block_res = sub_block();
  if (!try_block_res) {
    return NullOpt{};
  }

  Optional<CatchBlock> catch_block = NullOpt{};
  const auto& maybe_catch_token = iterator.peek();

  if (maybe_catch_token.type == TokenType::keyword_catch) {
    iterator.advance();

    const bool allow_empty = true;  //  @Note: Allow empty catch expression.
    auto catch_expr_res = expr(allow_empty);

    if (!catch_expr_res) {
      return NullOpt{};
    }

    auto catch_block_res = sub_block();
    if (!catch_block_res) {
      return NullOpt{};
    }

    catch_block = CatchBlock(maybe_catch_token, catch_expr_res.rvalue(), catch_block_res.rvalue());
  }

  auto end_err = consume(TokenType::keyword_end);
  if (end_err) {
    add_error(end_err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<TryStmt>(source_token,
    try_block_res.rvalue(), std::move(catch_block));

  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::stmt() {
  const auto& token = iterator.peek();

  switch (token.type) {
    case TokenType::keyword_return:
    case TokenType::keyword_break:
    case TokenType::keyword_continue:
      return control_stmt(token);

    case TokenType::keyword_for:
    case TokenType::keyword_parfor:
      return for_stmt(token);

    case TokenType::keyword_if:
      return if_stmt(token);

    case TokenType::keyword_switch:
      return switch_stmt(token);

    case TokenType::keyword_while:
      return while_stmt(token);

    case TokenType::keyword_try:
      return try_stmt(token);

    default: {
      if (is_command_stmt(token)) {
        return command_stmt(token);
      } else {
        return expr_stmt(token);
      }
    }
  }
}

bool AstGenerator::is_unary_prefix_expr(const mt::Token& curr_token) const {
  return represents_prefix_unary_operator(curr_token.type) &&
    (curr_token.type == TokenType::tilde ||
    can_precede_prefix_unary_operator(iterator.peek_prev().type));
}

bool AstGenerator::is_colon_subscript_expr(const mt::Token& curr_token) const {
  if (curr_token.type != TokenType::colon) {
    return false;
  }

  auto next_type = iterator.peek_next().type;
  return represents_grouping_terminator(next_type) || next_type == TokenType::comma;
}

bool AstGenerator::is_ignore_output_expr(const mt::Token& curr_token) const {
  //  [~] = func() | [~, a] = func()
  if (curr_token.type != TokenType::tilde) {
    return false;
  }

  const auto& next = iterator.peek_nth(1);
  return next.type == TokenType::comma || next.type == TokenType::right_bracket;
}

bool AstGenerator::is_command_stmt(const mt::Token& curr_token) const {
  const auto next_t = iterator.peek_next().type;

  return curr_token.type == TokenType::identifier &&
    (represents_literal(next_t) || next_t == TokenType::identifier);
}

Optional<ParseError> AstGenerator::consume(mt::TokenType type) {
  const auto& tok = iterator.peek();

  if (tok.type == type) {
    iterator.advance();
    return NullOpt{};
  } else {
    return Optional<ParseError>(make_error_expected_token_type(tok, &type, 1));
  }
}

Optional<ParseError> AstGenerator::consume_one_of(const mt::TokenType* types, int64_t num_types) {
  const auto& tok = iterator.peek();

  for (int64_t i = 0; i < num_types; i++) {
    if (types[i] == tok.type) {
      iterator.advance();
      return NullOpt{};
    }
  }

  return Optional<ParseError>(make_error_expected_token_type(tok, types, num_types));
}

void AstGenerator::add_error(mt::ParseError&& err) {
  parse_errors.emplace_back(std::move(err));
}

ParseError AstGenerator::make_error_reference_after_parens_reference_expr(const mt::Token& at_token) {
  const char* msg = "`()` indexing must appear last in an index expression.";
  return ParseError(text, at_token, msg);
}

ParseError AstGenerator::make_error_invalid_expr_token(const mt::Token& at_token) {
  const auto type_name = "`" + std::string(to_string(at_token.type)) + "`";
  const auto message = std::string("Token ") + type_name + " is not valid in expressions.";
  return ParseError(text, at_token, message);
}

ParseError AstGenerator::make_error_incomplete_expr(const mt::Token& at_token) {
  return ParseError(text, at_token, "Expression is incomplete.");
}

ParseError AstGenerator::make_error_invalid_assignment_target(const mt::Token& at_token) {
  return ParseError(text, at_token, "The expression on the left is not a valid target for assignment.");
}

ParseError AstGenerator::make_error_expected_lhs(const mt::Token& at_token) {
  return ParseError(text, at_token, "Expected an expression on the left hand side.");
}

ParseError AstGenerator::make_error_semicolon_delimiter_in_parens_grouping_expr(const mt::Token& at_token) {
  return ParseError(text, at_token,
    "`()` grouping expressions cannot contain semicolons or span multiple lines.");
}

ParseError AstGenerator::make_error_duplicate_otherwise_in_switch_stmt(const Token& at_token) {
  return ParseError(text, at_token, "Duplicate `otherwise` in `switch` statement.");
}

ParseError AstGenerator::make_error_expected_token_type(const mt::Token& at_token, const mt::TokenType* types,
                                                        int64_t num_types) {
  std::vector<std::string> type_strs;

  for (int64_t i = 0; i < num_types; i++) {
    std::string type_str = std::string("`") + to_string(types[i]) + "`";
    type_strs.emplace_back(type_str);
  }

  const auto expected_str = join(type_strs, ", ");
  std::string message = "Expected to receive one of these types: \n\n" + expected_str;
  message += "\n\nInstead, received: `";
  message += to_string(at_token.type);
  message += "`.";

  return ParseError(text, at_token, std::move(message));
}

}
