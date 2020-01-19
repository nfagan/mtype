#include "ast_gen.hpp"
#include "string.hpp"
#include "character.hpp"
#include <array>

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

Result<ParseErrors, BoxedAstNode> AstGenerator::parse(const std::vector<Token>& tokens, std::string_view txt) {
  iterator = TokenIterator(&tokens);
  text = txt;

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

Result<ParseErrors, std::vector<std::string_view>> AstGenerator::char_identifier_sequence(TokenType terminator) {
  std::vector<std::string_view> identifiers;
  ParseErrors errors;
  bool expect_identifier = true;

  while (iterator.has_next() && iterator.peek().type != terminator && errors.empty()) {
    const auto& tok = iterator.peek();

    if (expect_identifier && tok.type != TokenType::identifier) {
      std::array<TokenType, 2> possible_types{{TokenType::identifier, terminator}};
      errors.emplace_back(make_error_expected_token_type(tok, possible_types.data(), possible_types.size()));

    } else if (expect_identifier && tok.type == TokenType::identifier) {
      identifiers.push_back(tok.lexeme);
      expect_identifier = false;

    } else if (!expect_identifier && tok.type == TokenType::comma) {
      expect_identifier = true;

    } else {
      std::array<TokenType, 2> possible_types{{TokenType::comma, terminator}};
      errors.emplace_back(make_error_expected_token_type(tok, possible_types.data(), possible_types.size()));
    }

    if (errors.empty()) {
      iterator.advance();
    }
  }

  if (!errors.empty()) {
    return make_error<ParseErrors, std::vector<std::string_view>>(std::move(errors));
  }

  auto term_err = consume(terminator);
  if (term_err) {
    errors.emplace_back(term_err.rvalue());
    return make_error<ParseErrors, std::vector<std::string_view>>(std::move(errors));
  } else {
    return make_success<ParseErrors, std::vector<std::string_view>>(std::move(identifiers));
  }
}

Result<ParseErrors, FunctionHeader> AstGenerator::function_header() {
  auto output_res = function_outputs();
  if (!output_res) {
    return make_error<ParseErrors, FunctionHeader>(std::move(output_res.error));
  }

  if (!output_res.value.empty()) {
    auto err = consume(TokenType::equal);
    if (err) {
      ParseErrors errs;
      errs.emplace_back(err.rvalue());
      return make_error<ParseErrors, FunctionHeader>(std::move(errs));
    }
  }

  auto name_token = iterator.peek();
  auto name_res = char_identifier();
  if (!name_res) {
    ParseErrors errs;
    errs.emplace_back(name_res.error);
    return make_error<ParseErrors, FunctionHeader>(std::move(errs));
  }

  auto input_res = function_inputs();
  if (!input_res) {
    return make_error<ParseErrors, FunctionHeader>(std::move(input_res.error));
  }

  FunctionHeader header{name_token, name_res.value, std::move(output_res.value), std::move(input_res.value)};
  return make_success<ParseErrors, FunctionHeader>(std::move(header));
}

Result<ParseErrors, std::vector<std::string_view>> AstGenerator::function_inputs() {
  if (iterator.peek().type == TokenType::left_parens) {
    iterator.advance();
    return char_identifier_sequence(TokenType::right_parens);
  } else {
    return {};
  }
}

Result<ParseErrors, std::vector<std::string_view>> AstGenerator::function_outputs() {
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

    return make_success<ParseErrors, std::vector<std::string_view>>(std::move(outputs));
  } else {
    std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
    std::vector<ParseError> errors;
    errors.emplace_back(make_error_expected_token_type(tok, possible_types.data(), possible_types.size()));
    return make_error<ParseErrors, std::vector<std::string_view>>(std::move(errors));
  }
}

Result<ParseErrors, BoxedAstNode> AstGenerator::function_def() {
  iterator.advance();
  auto header_result = function_header();

  if (!header_result) {
    return make_error<ParseErrors, BoxedAstNode>(std::move(header_result.error));
  }

  auto success = std::make_unique<FunctionDef>(std::move(header_result.value), nullptr);
  return make_success<ParseErrors, BoxedAstNode>(std::move(success));
}

Result<ParseErrors, BoxedAstNode> AstGenerator::block() {
  ParseErrors errors;
  auto block_node = std::make_unique<Block>();

  while (iterator.has_next()) {
    Result<ParseErrors, BoxedAstNode> res;
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::new_line:
      case TokenType::null:
        iterator.advance();
        break;
      case TokenType::keyword_function:
        res = function_def();
        break;
      default:
        iterator.advance();
    }

    if (res) {
      if (res.value) {
        block_node->append(std::move(res.value));
      }
    } else {
      errors.insert(errors.end(), res.error.cbegin(), res.error.cend());
      std::array<TokenType, 1> possible_types{{TokenType::keyword_function}};
      iterator.advance_to_one(possible_types.data(), possible_types.size());
    }
  }

  if (errors.empty()) {
    return make_success<ParseErrors, BoxedAstNode>(std::move(block_node));
  } else {
    return make_error<ParseErrors, BoxedAstNode>(std::move(errors));
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
  //  @TODO: Recode `end` to `end` subscript reference.
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
  } else {
    std::vector<BoxedExpr> args;
    args.emplace_back(std::move(expr_res.value));
    auto subscript_expr = std::make_unique<SubscriptExpr>(source_token, SubscriptMethod::period, std::move(args));
    return make_success<ParseError, std::unique_ptr<SubscriptExpr>>(std::move(subscript_expr));
  }
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

    switch (tok.type) {
      case TokenType::period:
        sub_result = period_subscript_expr();
        break;
      case TokenType::left_parens:
        sub_result = non_period_subscript_expr(SubscriptMethod::parens, TokenType::right_parens);
        break;
      case TokenType::left_brace:
        sub_result = non_period_subscript_expr(SubscriptMethod::brace, TokenType::right_brace);
      default:
        break;
    }

    if (!sub_result) {
      return Result<ParseError, BoxedExpr>(std::move(sub_result.error));
    }

    if (prev_method == SubscriptMethod::parens && sub_result.value->method != SubscriptMethod::period) {
      return make_error<ParseError, BoxedExpr>(make_error_reference_after_parens_reference_expr(tok));
    } else {
      prev_method = sub_result.value->method;
      subscripts.emplace_back(std::move(sub_result.value));
    }
  }

  auto node = std::make_unique<IdentifierReferenceExpr>(main_ident_token, std::move(subscripts));
  return make_success<ParseError, BoxedExpr>(std::move(node));
}

Result<ParseError, BoxedExpr> AstGenerator::grouping_expr() {
  const auto& tok = iterator.peek();
  const TokenType terminator = grouping_terminator_for(tok.type);
  iterator.advance();

  const bool allow_empty = tok.type == TokenType::left_bracket || tok.type == TokenType::left_brace;
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
      auto component_expr = std::make_unique<GroupingExprComponent>(std::move(expr_res.value), delim);
      exprs.emplace_back(std::move(component_expr));
    }
  }

  auto err = consume(terminator);
  if (err) {
    return make_error<ParseError, BoxedExpr>(err.rvalue());
  } else {
    auto expr_node = std::make_unique<GroupingExpr>(tok, grouping_method_from_token_type(tok.type), std::move(exprs));
    return make_success<ParseError, BoxedExpr>(std::move(expr_node));
  }
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

    } else if (represents_grouping_initiator(tok.type)) {
      node_res = grouping_expr();

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

ParseError AstGenerator::make_error_expected_token_type(const mt::Token& at_token, const mt::TokenType* types,
                                                        int64_t num_types) {
  std::vector<std::string> type_strs;

  for (int64_t i = 0; i < num_types; i++) {
    std::string type_str = std::string("`") + to_string(types[i]) + "`";
    type_strs.emplace_back(type_str);
  }

  const auto expected_str = join(type_strs, ", ");
  std::string message = "Expected to receive one of these types: \n\n" + expected_str + "\n\nInstead, received: `";
  message += to_string(at_token.type);
  message += "`.";

  return ParseError(text, at_token, std::move(message));
}

}
