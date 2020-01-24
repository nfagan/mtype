#include "ast_gen.hpp"
#include "string.hpp"
#include <array>
#include <cassert>
#include <functional>
#include <set>

namespace mt {

Result<ParseErrors, std::unique_ptr<Block>>
AstGenerator::parse(const std::vector<Token>& tokens, std::string_view txt,
                    StringRegistry& registry, bool functions_are_end_terminated) {

  iterator = TokenIterator(&tokens);
  text = txt;
  is_end_terminated_function = functions_are_end_terminated;
  parse_errors.clear();
  string_registry = &registry;

  auto result = block();
  if (result) {
    return make_success<ParseErrors, std::unique_ptr<Block>>(result.rvalue());
  } else {
    return make_error<ParseErrors, std::unique_ptr<Block>>(std::move(parse_errors));
  }
}

Optional<std::string_view> AstGenerator::one_identifier() {
  auto tok = iterator.peek();
  auto err = consume(TokenType::identifier);

  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  } else {
    return Optional<std::string_view>(tok.lexeme);
  }
}

Optional<std::vector<Optional<int64_t>>> AstGenerator::anonymous_function_input_parameters() {
  std::vector<Optional<int64_t>> input_parameters;
  bool expect_parameter = true;

  while (iterator.has_next() && iterator.peek().type != TokenType::right_parens) {
    const auto& tok = iterator.peek();
    Optional<ParseError> err;

    if (expect_parameter) {
      std::array<TokenType, 2> possible_types{{TokenType::identifier, TokenType::tilde}};
      err = consume_one_of(possible_types.data(), possible_types.size());
    } else {
      err = consume(TokenType::comma);
    }

    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }

    if (tok.type == TokenType::identifier) {
      input_parameters.emplace_back(string_registry->register_string(tok.lexeme));

    } else if (tok.type == TokenType::tilde) {
      input_parameters.emplace_back(NullOpt{});
    }

    expect_parameter = !expect_parameter;
  }

  auto err = consume(TokenType::right_parens);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  return Optional<std::vector<Optional<int64_t>>>(std::move(input_parameters));
}

Optional<std::vector<std::string_view>> AstGenerator::function_identifier_components() {
  //  @Note: We can't just use the identical identifier sequence logic from below, because
  //  there's no one expected terminator here.
  std::vector<std::string_view> identifiers;
  bool expect_identifier = true;

  while (iterator.has_next()) {
    const auto& tok = iterator.peek();

    const auto expected_type = expect_identifier ? TokenType::identifier : TokenType::period;
    if (tok.type != expected_type) {
      if (expect_identifier) {
        add_error(make_error_expected_token_type(tok, &expected_type, 1));
        return NullOpt{};
      } else {
        break;
      }
    }

    iterator.advance();

    if (tok.type == TokenType::identifier) {
      identifiers.emplace_back(tok.lexeme);
    }

    expect_identifier = !expect_identifier;
  }

  return Optional<std::vector<std::string_view>>(std::move(identifiers));
}

Optional<std::vector<std::string_view>> AstGenerator::identifier_sequence(TokenType terminator) {
  std::vector<std::string_view> identifiers;
  bool expect_identifier = true;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    const auto& tok = iterator.peek();

    const auto expected_type = expect_identifier ? TokenType::identifier : TokenType::comma;
    auto err = consume(expected_type);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }

    if (expect_identifier) {
      identifiers.push_back(tok.lexeme);
    }

    expect_identifier = !expect_identifier;
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
  bool provided_outputs;
  auto output_res = function_outputs(&provided_outputs);
  if (!output_res) {
    return NullOpt{};
  }

  if (provided_outputs) {
    auto err = consume(TokenType::equal);
    if (err) {
      add_error(err.rvalue());
      return NullOpt{};
    }
  }

  const auto& name_token = iterator.peek();
  auto name_res = one_identifier();
  if (!name_res) {
    return NullOpt{};
  }

  auto input_res = function_inputs();
  if (!input_res) {
    return NullOpt{};
  }

  //  Register function definition strings.
  int64_t name = string_registry->register_string(name_res.value());
  auto outputs = string_registry->register_strings(output_res.value());
  auto inputs = string_registry->register_strings(input_res.value());

  FunctionHeader header(name_token, name, std::move(outputs), std::move(inputs));
  return Optional<FunctionHeader>(std::move(header));
}

Optional<std::vector<std::string_view>> AstGenerator::function_inputs() {
  if (iterator.peek().type == TokenType::left_parens) {
    iterator.advance();
    return identifier_sequence(TokenType::right_parens);
  } else {
    //  Function declarations without parentheses are valid and indicate 0 inputs.
    return Optional<std::vector<std::string_view>>(std::vector<std::string_view>());
  }
}

Optional<std::vector<std::string_view>> AstGenerator::function_outputs(bool* provided_outputs) {
  const auto& tok = iterator.peek();
  std::vector<std::string_view> outputs;
  *provided_outputs = false;

  if (tok.type == TokenType::left_bracket) {
    //  function [a, b, c] = example()
    iterator.advance();
    *provided_outputs = true;
    return identifier_sequence(TokenType::right_bracket);

  } else if (tok.type == TokenType::identifier) {
    //  Either: function a = example() or function example(). I.e., the next identifier is either
    //  the single output parameter of the function or the name of the function.
    const auto& next_tok = iterator.peek_nth(1);
    const bool is_single_output = next_tok.type == TokenType::equal;

    if (is_single_output) {
      *provided_outputs = true;
      outputs.push_back(tok.lexeme);
      iterator.advance();
    }

    return Optional<std::vector<std::string_view>>(std::move(outputs));

  } else {
    std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
    add_error(make_error_expected_token_type(tok, possible_types));
    return NullOpt{};
  }
}

Optional<std::unique_ptr<FunctionDef>> AstGenerator::function_def() {
  iterator.advance();
  auto header_result = function_header();

  if (!header_result) {
    return NullOpt{};
  }

  auto body_res = sub_block();
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
      case TokenType::type_annotation_macro: {
        auto type_res = type_annotation_macro(tok);
        if (type_res) {
          node = type_res.rvalue();
        } else {
          success = false;
        }
        break;
      }

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
      case TokenType::type_annotation_macro: {
        auto type_res = type_annotation_macro(tok);
        if (type_res) {
          node = type_res.rvalue();
        } else {
          success = false;
        }
        break;
      }

      case TokenType::keyword_function: {
        if (is_end_terminated_function) {
          auto def_res = function_def();
          if (def_res) {
            node = def_res.rvalue();
          } else {
            success = false;
          }
        } else {
          //  In a non-end terminated function, a function keyword here is actually the start of a
          //  new function at sibling-scope.
          should_proceed = false;
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
      const auto possible_types = sub_block_possible_types();
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
  //  Of the form a.b, where `b` is the identifier representing a presumed field of `a`.
  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  int64_t field_identifier = string_registry->register_string(ident_res.value());
  auto expr = std::make_unique<LiteralFieldReferenceExpr>(source_token, field_identifier);
  return Optional<BoxedExpr>(std::move(expr));
}

Optional<BoxedExpr> AstGenerator::dynamic_field_reference_expr(const Token& source_token) {
  //  Of the form a.(`expr`), where `expr` is assumed to evaluate to a field of `a`.
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

Optional<Subscript> AstGenerator::non_period_subscript(const Token& source_token,
                                                       mt::SubscriptMethod method,
                                                       mt::TokenType term) {
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

  Subscript subscript_expr(source_token, method, std::move(args));
  return Optional<Subscript>(std::move(subscript_expr));
}

Optional<Subscript> AstGenerator::period_subscript(const Token& source_token) {
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
      add_error(make_error_expected_token_type(tok, possible_types));
      return NullOpt{};
  }

  if (!expr_res) {
    return NullOpt{};
  }

  std::vector<BoxedExpr> args;
  args.emplace_back(expr_res.rvalue());

  Subscript subscript_expr(source_token, SubscriptMethod::period, std::move(args));
  return Optional<Subscript>(std::move(subscript_expr));
}

Optional<BoxedExpr> AstGenerator::identifier_reference_expr(const Token& source_token) {
  auto main_ident_res = one_identifier();
  if (!main_ident_res) {
    return NullOpt{};
  }

  std::vector<Subscript> subscripts;
  SubscriptMethod prev_method = SubscriptMethod::unknown;

  while (iterator.has_next()) {
    const auto& tok = iterator.peek();

    Optional<Subscript> sub_result = NullOpt{};
    bool has_subscript = true;

    if (tok.type == TokenType::period) {
      sub_result = period_subscript(tok);

    } else if (tok.type == TokenType::left_parens) {
      sub_result = non_period_subscript(tok, SubscriptMethod::parens, TokenType::right_parens);

    } else if (tok.type == TokenType::left_brace) {
      sub_result = non_period_subscript(tok, SubscriptMethod::brace, TokenType::right_brace);

    } else {
      has_subscript = false;
    }

    if (!has_subscript) {
      //  This is a simple variable reference or function call with no subscripts, e.g. `a;`
      break;
    } else if (!sub_result) {
      return NullOpt{};
    }

    auto sub_expr = sub_result.rvalue();

    if (prev_method == SubscriptMethod::parens && sub_expr.method != SubscriptMethod::period) {
      //  a(1, 1)(1) is an error, but a.('x')(1) is allowed.
      add_error(make_error_reference_after_parens_reference_expr(tok));
      return NullOpt{};
    } else {
      prev_method = sub_expr.method;
      subscripts.emplace_back(std::move(sub_expr));
    }
  }

  //  Register main identifier.
  int64_t primary_identifier = string_registry->register_string(main_ident_res.value());

  auto node = std::make_unique<IdentifierReferenceExpr>(source_token, primary_identifier, std::move(subscripts));
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::function_reference_expr(const Token& source_token) {
  //  @main
  iterator.advance(); //  consume '@'

  auto component_res = function_identifier_components();
  if (!component_res) {
    return NullOpt{};
  }

  auto component_identifiers = string_registry->register_strings(component_res.value());

  auto node = std::make_unique<FunctionReferenceExpr>(source_token, std::move(component_identifiers));
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::anonymous_function_expr(const mt::Token& source_token) {
  iterator.advance(); //  consume '@'

  //  Expect @(x, y, z) expr
  auto parens_err = consume(TokenType::left_parens);
  if (parens_err) {
    add_error(parens_err.rvalue());
    return NullOpt{};
  }

  auto input_res = anonymous_function_input_parameters();
  if (!input_res) {
    return NullOpt{};
  }

  auto body_res = expr();
  if (!body_res) {
    return NullOpt{};
  }

  auto input_identifiers = std::move(input_res.rvalue());

  auto params_err = check_anonymous_function_input_parameters_are_unique(source_token, input_identifiers);
  if (params_err) {
    add_error(params_err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<AnonymousFunctionExpr>(source_token, std::move(input_identifiers), body_res.rvalue());
  return Optional<BoxedExpr>(std::move(node));
}

Optional<BoxedExpr> AstGenerator::grouping_expr(const Token& source_token) {
  const TokenType terminator = grouping_terminator_for(source_token.type);
  iterator.advance();

  //  Concatenation constructions with brackets and braces can have empty expressions, e.g.
  //  [,,,,,,,,,;;;] and {,,,,,;;;;;} are valid and equivalent to [] and {}.
  const auto source_type = source_token.type;
  const bool allow_empty = source_type == TokenType::left_bracket || source_type == TokenType::left_brace;

  std::vector<GroupingExprComponent> exprs;

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
      delim = TokenType::semicolon;

    } else if (next.type != terminator) {
      std::array<TokenType, 4> possible_types{
        {TokenType::comma, TokenType::semicolon, TokenType::new_line, terminator}
      };
      add_error(make_error_expected_token_type(next, possible_types));
      return NullOpt{};
    }

    if (source_token.type == TokenType::left_parens && next.type != terminator) {
      //  (1, 2) or (1; 3) is not a valid expression.
      add_error(make_error_multiple_exprs_in_parens_grouping_expr(next));
      return NullOpt{};
    }

    if (expr_res.value()) {
      //  If non-empty.
      GroupingExprComponent grouping_component(expr_res.rvalue(), delim);
      exprs.emplace_back(std::move(grouping_component));
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

Optional<BoxedExpr> AstGenerator::ignore_argument_expr(const Token& source_token) {
  //  [~, a] = b(), or @(x, ~, z) y();
  iterator.advance();

  auto source_node = std::make_unique<IgnoreFunctionArgumentExpr>(source_token);
  return Optional<BoxedExpr>(std::move(source_node));
}

std::unique_ptr<UnaryOperatorExpr> AstGenerator::pending_unary_prefix_expr(const mt::Token& source_token) {
  iterator.advance();
  const auto op = unary_operator_from_token_type(source_token.type);
  //  Awaiting expr.
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

Optional<ParseError> AstGenerator::handle_postfix_unary_exprs(std::vector<BoxedExpr>& completed) {
  while (iterator.has_next()) {
    const auto& curr = iterator.peek();
    const auto& prev = iterator.peek_prev();

    if (represents_postfix_unary_operator(curr.type) && can_precede_postfix_unary_operator(prev.type)) {
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

Optional<BoxedExpr> AstGenerator::function_expr(const Token& source_token) {
  if (iterator.peek_next().type == TokenType::identifier) {
    return function_reference_expr(source_token);
  } else {
    return anonymous_function_expr(source_token);
  }
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

    } else if (is_ignore_argument_expr(tok)) {
      node_res = ignore_argument_expr(tok);

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
      node_res = function_expr(tok);

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
        std::array<TokenType, 2> possible_types{{TokenType::keyword_case, TokenType::keyword_otherwise}};
        add_error(make_error_expected_token_type(tok, possible_types));
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
  //  @TODO: Handle optional parenthetical initializer: for (i = 1:10) ** but not for ((i = 1:10))
  iterator.advance();

  //  for (i = 1:10)
  const bool is_wrapped_by_parens = iterator.peek().type == TokenType::left_parens;
  if (is_wrapped_by_parens) {
    iterator.advance();
  }

  auto loop_var_res = one_identifier();
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

  if (is_wrapped_by_parens) {
    auto parens_err = consume(TokenType::right_parens);
    if (parens_err) {
      return NullOpt{};
    }
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

  int64_t loop_variable_identifier = string_registry->register_string(loop_var_res.value());

  auto node = std::make_unique<ForStmt>(source_token, loop_variable_identifier,
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
  //  @TODO: Handle one line branch if (condition) d = 10; end
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

Optional<BoxedStmt> AstGenerator::variable_declaration_stmt(const Token& source_token) {
  iterator.advance();

  bool should_proceed = true;
  std::vector<std::int64_t> identifiers;

  while (iterator.has_next() && should_proceed) {
    const auto& tok = iterator.peek();

    switch (tok.type) {
      case TokenType::null:
      case TokenType::new_line:
      case TokenType::semicolon:
      case TokenType::comma:
        should_proceed = false;
        break;
      case TokenType::identifier:
        iterator.advance();
        identifiers.push_back(string_registry->register_string(tok.lexeme));
        break;
      default:
        //  e.g. global 1
        iterator.advance();
        const auto expected_type = TokenType::identifier;
        add_error(make_error_expected_token_type(tok, &expected_type, 1));
        return NullOpt{};
    }
  }

  const auto var_qualifier = variable_declaration_qualifier_from_token_type(source_token.type);
  auto node = std::make_unique<VariableDeclarationStmt>(source_token, var_qualifier, std::move(identifiers));
  return Optional<BoxedStmt>(std::move(node));
}

Optional<BoxedStmt> AstGenerator::command_stmt(const Token& source_token) {
  auto ident_res = one_identifier();
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

  int64_t command_identifier = string_registry->register_string(ident_res.value());

  auto node = std::make_unique<CommandStmt>(source_token, command_identifier, std::move(arguments));
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

    case TokenType::keyword_persistent:
    case TokenType::keyword_global:
      return variable_declaration_stmt(token);

    default: {
      if (is_command_stmt(token)) {
        return command_stmt(token);
      } else {
        return expr_stmt(token);
      }
    }
  }
}

Optional<BoxedTypeAnnot> AstGenerator::type_annotation_macro(const Token& source_token) {
  iterator.advance();
  const auto& dispatch_token = iterator.peek();
  Optional<BoxedTypeAnnot> annot_res;

  switch (dispatch_token.type) {
    case TokenType::left_bracket:
    case TokenType::identifier:
      annot_res = inline_type_annotation(dispatch_token);
      break;
    default:
      annot_res = type_annotation(dispatch_token);
  }

  if (!annot_res) {
    return NullOpt{};
  }

  auto node = std::make_unique<TypeAnnotMacro>(source_token, annot_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::inline_type_annotation(const mt::Token& source_token) {
  auto type_res = type(source_token);
  if (!type_res) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end_type);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<InlineType>(type_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_annotation(const mt::Token& source_token) {
  switch (source_token.type) {
    case TokenType::keyword_begin:
      return type_begin(source_token);

    case TokenType::keyword_given:
      return type_given(source_token);

    case TokenType::keyword_let:
      return type_let(source_token);

    default:
      auto possible_types = type_annotation_block_possible_types();
      add_error(make_error_expected_token_type(source_token, possible_types));
      return NullOpt{};
  }
}

Optional<BoxedTypeAnnot> AstGenerator::type_let(const mt::Token& source_token) {
  iterator.advance();

  auto ident_res = one_identifier();
  if (!ident_res) {
    return NullOpt{};
  }

  auto equal_err = consume(TokenType::equal);
  if (equal_err) {
    add_error(equal_err.rvalue());
    return NullOpt{};
  }

  auto equal_to_type_res = type(iterator.peek());
  if (!equal_to_type_res) {
    return NullOpt{};
  }

  int64_t identifier = string_registry->register_string(ident_res.value());

  auto let_node = std::make_unique<TypeLet>(source_token, identifier, equal_to_type_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(let_node));
}

Optional<BoxedTypeAnnot> AstGenerator::type_given(const mt::Token& source_token) {
  iterator.advance();

  const auto& ident_token = iterator.peek();
  auto identifier_res = type_variable_identifiers(ident_token);
  if (!identifier_res) {
    return NullOpt{};
  }

  if (identifier_res.value().empty()) {
    add_error(make_error_expected_non_empty_type_variable_identifiers(ident_token));
    return NullOpt{};
  }

  const auto& decl_token = iterator.peek();
  Optional<BoxedTypeAnnot> decl_res;

  switch (decl_token.type) {
    case TokenType::keyword_let:
      decl_res = type_let(decl_token);
      break;
    default: {
      std::array<TokenType, 1> possible_types{{TokenType::keyword_let}};
      add_error(make_error_expected_token_type(source_token, possible_types));
      return NullOpt{};
    }
  }

  if (!decl_res) {
    return NullOpt{};
  }

  auto identifiers = string_registry->register_strings(identifier_res.value());

  auto node = std::make_unique<TypeGiven>(source_token, std::move(identifiers), decl_res.rvalue());
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<std::vector<std::string_view>> AstGenerator::type_variable_identifiers(const mt::Token& source_token) {
  std::vector<std::string_view> type_variable_identifiers;

  if (source_token.type == TokenType::less) {
    iterator.advance();
    auto type_variable_res = identifier_sequence(TokenType::greater);
    if (!type_variable_res) {
      return NullOpt{};
    }
    type_variable_identifiers = type_variable_res.rvalue();

  } else if (source_token.type == TokenType::identifier) {
    iterator.advance();
    type_variable_identifiers.emplace_back(source_token.lexeme);

  } else {
    std::array<TokenType, 2> possible_types{{TokenType::less, TokenType::identifier}};
    add_error(make_error_expected_token_type(source_token, possible_types));
    return NullOpt{};
  }

  return Optional<std::vector<std::string_view>>(std::move(type_variable_identifiers));
}

Optional<BoxedTypeAnnot> AstGenerator::type_begin(const mt::Token& source_token) {
  iterator.advance();

  const bool is_exported = iterator.peek().type == TokenType::keyword_export;
  if (is_exported) {
    iterator.advance();
  }

  std::vector<BoxedTypeAnnot> contents;
  bool had_error = false;

  while (iterator.has_next() && iterator.peek().type != TokenType::keyword_end_type) {
    const auto& token = iterator.peek();

    switch (token.type) {
      case TokenType::new_line:
      case TokenType::null:
        iterator.advance();
        break;
      default: {
        auto annot_res = type_annotation(token);
        if (annot_res) {
          contents.emplace_back(annot_res.rvalue());
        } else {
          had_error = true;
          const auto possible_types = type_annotation_block_possible_types();
          iterator.advance_to_one(possible_types.data(), possible_types.size());
        }
      }
    }
  }

  if (had_error) {
    return NullOpt{};
  }

  auto err = consume(TokenType::keyword_end_type);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  auto node = std::make_unique<TypeBegin>(source_token, is_exported, std::move(contents));
  return Optional<BoxedTypeAnnot>(std::move(node));
}

Optional<BoxedType> AstGenerator::type(const mt::Token& source_token) {
  switch (source_token.type) {
    case TokenType::left_bracket:
      return function_type(source_token);

    case TokenType::identifier:
      return scalar_type(source_token);

    default: {
      std::array<TokenType, 2> possible_types{{TokenType::left_bracket, TokenType::identifier}};
      add_error(make_error_expected_token_type(source_token, possible_types));
      return NullOpt{};
    }
  }
}

Optional<std::vector<BoxedType>> AstGenerator::type_sequence(TokenType terminator) {
  std::vector<BoxedType> types;

  while (iterator.has_next() && iterator.peek().type != terminator) {
    const auto& source_token = iterator.peek();

    auto type_res = type(source_token);
    if (!type_res) {
      return NullOpt{};
    }

    types.emplace_back(type_res.rvalue());

    const auto next_type = iterator.peek().type;
    if (next_type == TokenType::comma) {
      iterator.advance();

    } else if (next_type != terminator) {
      std::array<TokenType, 2> possible_types{{TokenType::comma, terminator}};
      add_error(make_error_expected_token_type(iterator.peek(), possible_types));
      return NullOpt{};
    }
  }

  auto err = consume(terminator);
  if (err) {
    add_error(err.rvalue());
    return NullOpt{};
  }

  return Optional<std::vector<BoxedType>>(std::move(types));
}

Optional<BoxedType> AstGenerator::function_type(const mt::Token& source_token) {
  iterator.advance();

  auto output_res = type_sequence(TokenType::right_bracket);
  if (!output_res) {
    return NullOpt{};
  }

  auto equal_err = consume(TokenType::equal);
  if (equal_err) {
    add_error(equal_err.rvalue());
    return NullOpt{};
  }

  auto parens_err = consume(TokenType::left_parens);
  if (parens_err) {
    add_error(parens_err.rvalue());
    return NullOpt{};
  }

  auto input_res = type_sequence(TokenType::right_parens);
  if (!input_res) {
    return NullOpt{};
  }

  auto type_node = std::make_unique<FunctionType>(source_token,
    output_res.rvalue(), input_res.rvalue());
  return Optional<BoxedType>(std::move(type_node));
}

Optional<BoxedType> AstGenerator::scalar_type(const mt::Token& source_token) {
  iterator.advance(); //  consume lexeme

  std::vector<BoxedType> arguments;

  if (iterator.peek().type == TokenType::less) {
    iterator.advance();

    auto argument_res = type_sequence(TokenType::greater);
    if (!argument_res) {
      return NullOpt{};
    } else {
      arguments = argument_res.rvalue();
    }
  }

  int64_t identifier = string_registry->register_string(source_token.lexeme);

  auto node = std::make_unique<ScalarType>(source_token, identifier, std::move(arguments));
  return Optional<BoxedType>(std::move(node));
}

std::array<TokenType, 3> AstGenerator::type_annotation_block_possible_types() {
  return std::array<TokenType, 3>{
    {TokenType::keyword_begin, TokenType::keyword_given, TokenType::keyword_let
  }};
}

std::array<TokenType, 5> AstGenerator::sub_block_possible_types() {
  return {
    {TokenType::keyword_if, TokenType::keyword_for, TokenType::keyword_while,
    TokenType::keyword_try, TokenType::keyword_switch}
  };
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

bool AstGenerator::is_ignore_argument_expr(const mt::Token& curr_token) const {
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

Optional<ParseError>
AstGenerator::check_anonymous_function_input_parameters_are_unique(const Token& source_token,
                                                                   const std::vector<Optional<int64_t>>& inputs) const {
  std::set<int64_t> uniques;
  for (const auto& input : inputs) {
    if (!input) {
      //  Input argument is ignored (~)
      continue;
    }

    auto orig_size = uniques.size();
    uniques.insert(input.value());

    if (orig_size == uniques.size()) {
      //  This value is not new.
      return Optional<ParseError>(make_error_duplicate_input_parameter_in_expr(source_token));
    }
  }
  return NullOpt{};
}

void AstGenerator::add_error(mt::ParseError&& err) {
  parse_errors.emplace_back(std::move(err));
}

ParseError AstGenerator::make_error_reference_after_parens_reference_expr(const mt::Token& at_token) const {
  const char* msg = "`()` indexing must appear last in an index expression.";
  return ParseError(text, at_token, msg);
}

ParseError AstGenerator::make_error_invalid_expr_token(const mt::Token& at_token) const {
  const auto type_name = "`" + std::string(to_string(at_token.type)) + "`";
  const auto message = std::string("Token ") + type_name + " is not permitted in expressions.";
  return ParseError(text, at_token, message);
}

ParseError AstGenerator::make_error_incomplete_expr(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Expression is incomplete.");
}

ParseError AstGenerator::make_error_invalid_assignment_target(const mt::Token& at_token) const {
  return ParseError(text, at_token, "The expression on the left is not a valid target for assignment.");
}

ParseError AstGenerator::make_error_expected_lhs(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Expected an expression on the left hand side.");
}

ParseError AstGenerator::make_error_multiple_exprs_in_parens_grouping_expr(const mt::Token& at_token) const {
  return ParseError(text, at_token,
    "`()` grouping expressions cannot contain multiple sub-expressions or span multiple lines.");
}

ParseError AstGenerator::make_error_duplicate_otherwise_in_switch_stmt(const Token& at_token) const {
  return ParseError(text, at_token, "Duplicate `otherwise` in `switch` statement.");
}

ParseError AstGenerator::make_error_expected_non_empty_type_variable_identifiers(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Expected a non-empty list of identifiers.");
}

ParseError AstGenerator::make_error_duplicate_input_parameter_in_expr(const mt::Token& at_token) const {
  return ParseError(text, at_token, "Anonymous function contains a duplicate input parameter identifier.");
}

ParseError AstGenerator::make_error_expected_token_type(const mt::Token& at_token, const mt::TokenType* types,
                                                        int64_t num_types) const {
  std::vector<std::string> type_strs;

  for (int64_t i = 0; i < num_types; i++) {
    std::string type_str = std::string("`") + to_string(types[i]) + "`";
    type_strs.emplace_back(type_str);
  }

  const auto expected_str = join(type_strs, ", ");
  std::string message = "Expected to receive one of these types: \n\n" + expected_str;
  message += (std::string("\n\nInstead, received: `") + to_string(at_token.type) + "`.");

  return ParseError(text, at_token, std::move(message));
}

}
