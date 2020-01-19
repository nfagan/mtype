#include "ast_gen.hpp"
#include "string.hpp"
#include "character.hpp"
#include <array>
#include <cassert>

namespace mt {

void ParseError::show() const {
  if (!message.empty()) {
    const bool is_null = at_token.type == TokenType::null;
    const auto start = is_null ? 0 : at_token.lexeme.data() - text.data();
    const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text.data();

    auto msg = mark_text_with_message_and_context(text, start, stop, 100, message);
    std::cout << msg << std::endl;
  }
}

namespace {
  inline ParseErrors parse_error_to_errors(ParseError&& err) {
    ParseErrors res;
    res.emplace_back(std::move(err));
    return res;
  }
}

Result<ParseErrors, std::unique_ptr<Block>>
AstGenerator::parse(const std::vector<Token>& tokens, std::string_view txt) {
  iterator = TokenIterator(&tokens);
  text = txt;
  is_end_terminated_function = true;

  return block();
}

Result<ParseError, std::string_view> AstGenerator::char_identifier() {
  auto tok = iterator.peek();
  auto err = consume(TokenType::identifier);

  if (err) {
    return make_error<ParseError, std::string_view>(err.rvalue());
  } else {
    return make_success<ParseError, std::string_view>(tok.lexeme);
  }
}

Result<ParseError, std::vector<std::string_view>>
AstGenerator::char_identifier_sequence(TokenType terminator) {
  std::vector<std::string_view> identifiers;
  bool expect_identifier = true;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    const auto& tok = iterator.peek();

    if (expect_identifier && tok.type != TokenType::identifier) {
      std::array<TokenType, 2> possible_types{{TokenType::identifier, terminator}};
      auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
      return make_error<ParseError, std::vector<std::string_view>>(std::move(err));

    } else if (expect_identifier && tok.type == TokenType::identifier) {
      identifiers.push_back(tok.lexeme);
      expect_identifier = false;

    } else if (!expect_identifier && tok.type == TokenType::comma) {
      expect_identifier = true;

    } else {
      std::array<TokenType, 2> possible_types{{TokenType::comma, terminator}};
      auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
      return make_error<ParseError, std::vector<std::string_view>>(std::move(err));
    }

    iterator.advance();
  }

  auto term_err = consume(terminator);
  if (term_err) {
    return make_error<ParseError, std::vector<std::string_view>>(term_err.rvalue());
  } else {
    return make_success<ParseError, std::vector<std::string_view>>(std::move(identifiers));
  }
}

Result<ParseError, FunctionHeader> AstGenerator::function_header() {
  auto output_res = function_outputs();
  if (!output_res) {
    return make_error<ParseError, FunctionHeader>(std::move(output_res.error));
  }

  if (!output_res.value.empty()) {
    auto err = consume(TokenType::equal);
    if (err) {
      return make_error<ParseError, FunctionHeader>(err.rvalue());
    }
  }

  const auto& name_token = iterator.peek();
  auto name_res = char_identifier();
  if (!name_res) {
    return make_error<ParseError, FunctionHeader>(std::move(name_res.error));
  }

  auto input_res = function_inputs();
  if (!input_res) {
    return make_error<ParseError, FunctionHeader>(std::move(input_res.error));
  }

  FunctionHeader header{name_token, name_res.value, std::move(output_res.value), std::move(input_res.value)};
  return make_success<ParseError, FunctionHeader>(std::move(header));
}

Result<ParseError, std::vector<std::string_view>> AstGenerator::function_inputs() {
  if (iterator.peek().type == TokenType::left_parens) {
    iterator.advance();
    return char_identifier_sequence(TokenType::right_parens);
  } else {
    return {};
  }
}

Result<ParseError, std::vector<std::string_view>> AstGenerator::function_outputs() {
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

    return make_success<ParseError, std::vector<std::string_view>>(std::move(outputs));
  } else {
    std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
    auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
    return make_error<ParseError, std::vector<std::string_view>>(std::move(err));
  }
}

Result<ParseErrors, std::unique_ptr<FunctionDef>> AstGenerator::function_def() {
  iterator.advance();
  auto header_result = function_header();

  if (!header_result) {
    auto errs = parse_error_to_errors(std::move(header_result.error));
    return make_error<ParseErrors, std::unique_ptr<FunctionDef>>(std::move(errs));
  }

  auto body_res = block();
  if (!body_res) {
    return make_error<ParseErrors, std::unique_ptr<FunctionDef>>(std::move(body_res.error));
  }

  if (is_end_terminated_function) {
    auto err = consume(TokenType::keyword_end);
    if (err) {
      return make_error<ParseErrors, std::unique_ptr<FunctionDef>>(parse_error_to_errors(err.rvalue()));
    }
  }

  auto success = std::make_unique<FunctionDef>(std::move(header_result.value),
    std::move(body_res.value));
  return make_success<ParseErrors, std::unique_ptr<FunctionDef>>(std::move(success));
}

Result<ParseErrors, std::unique_ptr<Block>> AstGenerator::block() {
  ParseErrors errors;
  auto block_node = std::make_unique<Block>();
  bool should_proceed = true;

  while (iterator.has_next() && should_proceed) {
    BoxedAstNode node;
    ParseErrors error;
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
          node = std::move(def_res.value);
        } else {
          success = false;
          error = std::move(def_res.error);
        }
        break;
      }
      default: {
        auto stmt_res = stmt();
        if (stmt_res) {
          node = std::move(stmt_res.value);
        } else {
          success = false;
          error = std::move(stmt_res.error);
        }
      }
    }

    if (!success) {
      errors.insert(errors.end(), error.cbegin(), error.cend());
      std::array<TokenType, 1> possible_types{{TokenType::keyword_function}};
      iterator.advance_to_one(possible_types.data(), possible_types.size());

    } else if (node) {
      block_node->append(std::move(node));
    }
  }

  if (errors.empty()) {
    return make_success<ParseErrors, std::unique_ptr<Block>>(std::move(block_node));
  } else {
    return make_error<ParseErrors, std::unique_ptr<Block>>(std::move(errors));
  }
}

Result<ParseError, BoxedExpr> AstGenerator::literal_field_reference_expr() {
  const auto& tok = iterator.peek();
  auto ident_res = char_identifier();
  if (!ident_res) {
    return make_error<ParseError, BoxedExpr>(std::move(ident_res.error));
  }

  auto expr = std::make_unique<LiteralFieldReferenceExpr>(tok);
  return make_success<ParseError, BoxedExpr>(std::move(expr));
}

Result<ParseError, BoxedExpr> AstGenerator::dynamic_field_reference_expr() {
  iterator.advance(); //  consume (
  const auto& source_tok = iterator.peek();

  auto expr_res = expr();
  if (!expr_res) {
    return make_error<ParseError, BoxedExpr>(std::move(expr_res.error));
  }

  auto err = consume(TokenType::right_parens);
  if (err) {
    return make_error<ParseError, BoxedExpr>(err.rvalue());
  }

  auto expr = std::make_unique<DynamicFieldReferenceExpr>(source_tok, std::move(expr_res.value));
  return make_success<ParseError, BoxedExpr>(std::move(expr));
}

Result<ParseError, std::unique_ptr<SubscriptExpr>>
AstGenerator::non_period_subscript_expr(mt::SubscriptMethod method, mt::TokenType term) {
  //  @TODO: Recode `end` to `end subscript reference`.
  const auto& source_token = iterator.peek();
  iterator.advance(); //  consume '(' or '{'

  std::vector<BoxedExpr> args;

  while (iterator.has_next() && iterator.peek().type != term) {
    auto expr_res = expr();
    if (!expr_res) {
      return make_error<ParseError, std::unique_ptr<SubscriptExpr>>(std::move(expr_res.error));
    }

    args.emplace_back(std::move(expr_res.value));

    if (iterator.peek().type == TokenType::comma) {
      iterator.advance();
    } else {
      break;
    }
  }

  auto err = consume(term);
  if (err) {
    return make_error<ParseError, std::unique_ptr<SubscriptExpr>>(err.rvalue());
  }

  auto sub = std::make_unique<SubscriptExpr>(source_token, method, std::move(args));
  return make_success<ParseError, std::unique_ptr<SubscriptExpr>>(std::move(sub));
}

Result<ParseError, std::unique_ptr<SubscriptExpr>> AstGenerator::period_subscript_expr() {
  const auto& source_token = iterator.peek();
  iterator.advance(); //  consume '.'

  const auto& tok = iterator.peek();
  Result<ParseError, BoxedExpr> expr_res;

  switch (tok.type) {
    case TokenType::identifier:
      expr_res = literal_field_reference_expr();
      break;
    case TokenType::left_parens:
      expr_res = dynamic_field_reference_expr();
      break;
    default:
      std::array<TokenType, 2> possible_types{{TokenType::identifier, TokenType::left_parens}};
      auto err = make_error_expected_token_type(tok, possible_types.data(), possible_types.size());
      return make_error<ParseError, std::unique_ptr<SubscriptExpr>>(std::move(err));
  }

  if (!expr_res) {
    return make_error<ParseError, std::unique_ptr<SubscriptExpr>>(std::move(expr_res.error));
  }

  std::vector<BoxedExpr> args;
  args.emplace_back(std::move(expr_res.value));
  auto subscript_expr = std::make_unique<SubscriptExpr>(source_token, SubscriptMethod::period, std::move(args));

  return make_success<ParseError, std::unique_ptr<SubscriptExpr>>(std::move(subscript_expr));
}

Result<ParseError, BoxedExpr> AstGenerator::identifier_reference_expr() {
  const auto& main_ident_token = iterator.peek();

  auto main_ident_res = char_identifier();
  if (!main_ident_res) {
    return make_error<ParseError, BoxedExpr>(std::move(main_ident_res.error));
  }

  std::vector<std::unique_ptr<SubscriptExpr>> subscripts;
  SubscriptMethod prev_method = SubscriptMethod::unknown;

  while (iterator.has_next()) {
    const auto& tok = iterator.peek();
    Result<ParseError, std::unique_ptr<SubscriptExpr>> sub_result;
    bool has_subscript = true;

    if (tok.type == TokenType::period) {
      sub_result = period_subscript_expr();

    } else if (tok.type == TokenType::left_parens) {
      sub_result = non_period_subscript_expr(SubscriptMethod::parens, TokenType::right_parens);

    } else if (tok.type == TokenType::left_brace) {
      sub_result = non_period_subscript_expr(SubscriptMethod::brace, TokenType::right_brace);

    } else {
      has_subscript = false;
    }

    if (!sub_result) {
      return Result<ParseError, BoxedExpr>(std::move(sub_result.error));

    } else if (!has_subscript) {
      break;
    }

    auto& sub_expr = sub_result.value;

    if (prev_method == SubscriptMethod::parens && sub_expr->method != SubscriptMethod::period) {
      return make_error<ParseError, BoxedExpr>(make_error_reference_after_parens_reference_expr(tok));
    } else {
      prev_method = sub_expr->method;
      subscripts.emplace_back(std::move(sub_expr));
    }
  }

  auto node = std::make_unique<IdentifierReferenceExpr>(main_ident_token, std::move(subscripts));
  return make_success<ParseError, BoxedExpr>(std::move(node));
}

Result<ParseError, BoxedExpr> AstGenerator::grouping_expr(const Token& source_token) {
  const TokenType terminator = grouping_terminator_for(source_token.type);
  iterator.advance();

  const bool allow_empty = source_token.type == TokenType::left_bracket ||
    source_token.type == TokenType::left_brace;
  std::vector<std::unique_ptr<GroupingExprComponent>> exprs;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    auto expr_res = expr(allow_empty);
    if (!expr_res) {
      return expr_res;
    }

    const auto& next = iterator.peek();
    TokenType delim = TokenType::comma;

    if (next.type == TokenType::comma) {
      iterator.advance();

    } else if (next.type == TokenType::semicolon) {
      delim = TokenType::semicolon;
      iterator.advance();

    } else if (next.type != terminator) {
      std::array<TokenType, 3> possible_types{{TokenType::comma, TokenType::semicolon, terminator}};
      auto err = make_error_expected_token_type(next, possible_types.data(), possible_types.size());
      return make_error<ParseError, BoxedExpr>(std::move(err));
    }

    if (expr_res.value) {
      //  If non-empty.
      auto component_expr = std::make_unique<GroupingExprComponent>(std::move(expr_res.value), delim);
      exprs.emplace_back(std::move(component_expr));
    }
  }

  auto err = consume(terminator);
  if (err) {
    return make_error<ParseError, BoxedExpr>(err.rvalue());
  }

  const auto grouping_method = grouping_method_from_token_type(source_token.type);
  auto expr_node = std::make_unique<GroupingExpr>(source_token, grouping_method, std::move(exprs));
  return make_success<ParseError, BoxedExpr>(std::move(expr_node));
}

Result<ParseError, BoxedExpr> AstGenerator::literal_expr(const Token& source_token) {
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

  return make_success<ParseError, BoxedExpr>(std::move(expr_node));
}

Result<ParseError, BoxedExpr> AstGenerator::ignore_output_expr(const Token& source_token) {
  iterator.advance();

  auto source_node = std::make_unique<IgnoreFunctionOutputArgumentExpr>(source_token);
  return make_success<ParseError, BoxedExpr>(std::move(source_node));
}

Result<ParseError, BoxedExpr> AstGenerator::expr(bool allow_empty) {
  std::vector<std::unique_ptr<BinaryOperatorExpr>> pending_binaries;
  std::vector<std::unique_ptr<BinaryOperatorExpr>> binaries;
  std::vector<std::unique_ptr<UnaryOperatorExpr>> unaries;
  std::vector<BoxedExpr> completed;

  while (iterator.has_next()) {
    Result<ParseError, BoxedExpr> node_res;
    const auto& tok = iterator.peek();

    if (tok.type == TokenType::identifier) {
      node_res = identifier_reference_expr();

    } else if (is_ignore_output_expr(tok)) {
      node_res = ignore_output_expr(tok);

    } else if (represents_grouping_initiator(tok.type)) {
      node_res = grouping_expr(tok);

    } else if (represents_literal(tok.type)) {
      node_res = literal_expr(tok);

    } else if (represents_expr_terminator(tok.type)) {
      break;

    } else {
      auto err = make_error_invalid_expr_token(tok);
      return make_error<ParseError, BoxedExpr>(std::move(err));
    }

    if (!node_res) {
      return node_res;
    }

    if (node_res.value) {
      completed.emplace_back(std::move(node_res.value));
    }
  }

  if (completed.size() == 1) {
    return make_success<ParseError, BoxedExpr>(std::move(completed.back()));

  } else {
    const bool is_complete = completed.empty() && pending_binaries.empty() && binaries.empty() && unaries.empty();
    const bool is_valid = is_complete && allow_empty;

    if (!is_valid) {
      return make_error<ParseError, BoxedExpr>(make_error_incomplete_expr(iterator.peek()));
    } else {
      //  @TODO: Make an empty expression class.
      return make_success<ParseError, BoxedExpr>(nullptr);
    }
  }
}

Result<ParseError, BoxedStmt> AstGenerator::assignment_stmt(BoxedExpr lhs, const Token& initial_token) {
  iterator.advance(); //  consume =

  if (!lhs->is_valid_assignment_target()) {
    auto err = make_error_invalid_assignment_target(initial_token);
    return make_error<ParseError, BoxedStmt>(std::move(err));
  }

  auto rhs_res = expr();
  if (!rhs_res) {
    return make_error<ParseError, BoxedStmt>(std::move(rhs_res.error));
  }

  auto assign_stmt = std::make_unique<AssignmentStmt>(std::move(lhs), std::move(rhs_res.value));
  return make_success<ParseError, BoxedStmt>(std::move(assign_stmt));
}

Result<ParseError, BoxedStmt> AstGenerator::expr_stmt() {
  const auto& initial_token = iterator.peek();
  auto expr_res = expr();

  if (!expr_res) {
    return make_error<ParseError, BoxedStmt>(std::move(expr_res.error));
  }

  const auto& curr_token = iterator.peek();

  if (curr_token.type == TokenType::equal) {
    return assignment_stmt(std::move(expr_res.value), initial_token);
  }

  auto expr_stmt = std::make_unique<ExprStmt>(std::move(expr_res.value));
  return make_success<ParseError, BoxedStmt>(std::move(expr_stmt));
}

Result<ParseErrors, BoxedAstNode> AstGenerator::stmt() {
  const auto& token = iterator.peek();

  switch (token.type) {
    default:
      auto res = expr_stmt();
      if (res) {
        return make_success<ParseErrors, BoxedAstNode>(std::move(res.value));
      } else {
        return make_error<ParseErrors, BoxedAstNode>(parse_error_to_errors(std::move(res.error)));
      }
  }
}

bool AstGenerator::is_ignore_output_expr(const mt::Token& curr_token) const {
  //  [~] = func() | [~, a] = func()
  if (curr_token.type != TokenType::tilde) {
    return false;
  }

  const auto& next = iterator.peek_nth(1);
  return next.type == TokenType::comma || next.type == TokenType::right_bracket;
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

ParseError AstGenerator::make_error_reference_after_parens_reference_expr(const mt::Token& at_token) {
  const char* msg = "()-indexing must appear last in an index expression.";
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
