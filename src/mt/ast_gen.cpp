#include "ast_gen.hpp"
#include "string.hpp"
#include "character.hpp"
#include <array>

namespace mt {

void ParseError::show() const {
  if (!message.empty()) {
    const auto start = at_token.lexeme.data() - text.data();
    const auto stop = at_token.lexeme.data() + at_token.lexeme.size() - text.data();

    std::cout << "Start is: " << start << std::endl;
    std::cout << "Stop is" << stop << std::endl;
    std::cout << "Message is: " << message << std::endl;

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
      return make_error<ParseErrors, FunctionHeader>(err);
    }
  }

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

  FunctionHeader header{name_res.value, std::move(output_res.value), std::move(input_res.value)};
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

  std::array<TokenType, 1> possible_types{{TokenType::keyword_function}};

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
      for (auto&& err : res.error) {
        errors.emplace_back(err);
      }
      iterator.advance_to_one(possible_types.data(), possible_types.size());
    }
  }

  if (errors.empty()) {
    return make_success<ParseErrors, BoxedAstNode>(std::move(block_node));
  } else {
    return make_error<ParseErrors, BoxedAstNode>(std::move(errors));
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

ParseError AstGenerator::make_error_expected_token_type(const mt::Token& at_token, const mt::TokenType* types,
                                                        int64_t num_types) {
  std::vector<std::string> type_strs;

  for (int64_t i = 0; i < num_types; i++) {
    type_strs.emplace_back(to_string(types[i]));
  }

  const auto expected_str = join(type_strs, ", ");
  std::string message = "Expected to receive one of these types: \n\n" + expected_str + "\n\nInstead, received: `";
  message += to_string(at_token.type);
  message += "`.";

  return ParseError(text, at_token, std::move(message));
}

}
