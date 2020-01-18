#include "scan.hpp"
#include "character_traits.hpp"
#include "keyword.hpp"
#include "string.hpp"

namespace mt {

ScanResult Scanner::scan(const std::string& str) {
  return scan(str.c_str(), str.size());
}

ScanResult Scanner::scan(const char* text, int64_t len) {
  iterator = CharacterIterator(text, len);
  std::vector<Token> tokens;
  ScanErrors errors;

  while (iterator.has_next()) {
    const auto c = iterator.peek();

    if (is_alpha(c)) {
      tokens.push_back(identifier_or_keyword_token());

    } else if (is_digit(c) || (c == '.' && is_digit(iterator.peek_next()))) {
      tokens.push_back(number_literal_token());

    } else if (c == '\n') {
      tokens.push_back({TokenType::new_line, make_lexeme(iterator.next_index(), 1)});
      iterator.advance();

    } else if (c == '"') {
      auto token_result = string_literal_token(TokenType::string_literal, Character('"'));
      check_add_token(token_result, errors, tokens);

    } else if (c == '\'' && !can_precede_apostrophe(iterator.peek_previous())) {
      auto token_result = string_literal_token(TokenType::char_literal, Character('\''));
      check_add_token(token_result, errors, tokens);

    } else {
      iterator.advance();
    }
  }

  tokens.push_back({TokenType::null, std::string_view()});

  if (errors.errors.empty()) {
    return make_success<ScanErrors, std::vector<Token>>(std::move(tokens));
  } else {
    return make_error<ScanErrors, std::vector<Token>>(std::move(errors));
  }
}

std::string_view Scanner::make_lexeme(int64_t offset, int64_t len) const {
  return std::string_view(iterator.data() + offset, len);
}

void Scanner::check_add_token(mt::Result<mt::ScanError, mt::Token>& res, ScanErrors& errs, std::vector<Token>& tokens) {
  if (res) {
    tokens.push_back(res.value);
  } else {
    errs.errors.push_back(res.error);
  }
}

Result<ScanError, Token> Scanner::string_literal_token(mt::TokenType type, const mt::Character& terminator) {
  iterator.advance(); //  consume ' or "
  int64_t start = iterator.next_index();

  while (iterator.has_next()) {
    auto c = iterator.peek();

    if (c == terminator) {
      if (iterator.peek_next() == terminator) {
        iterator.advance();
      } else {
        break;
      }
    }

    iterator.advance();
  }

  if (!iterator.has_next()) {
    return make_error_unterminated_string_literal(start - 1);
  }

  Token result{type, make_lexeme(start, iterator.next_index() - start)};
  iterator.advance(); //  consume closing terminator.
  return make_success<ScanError, Token>(result);
}

Result<ScanError, Token> Scanner::make_error_unterminated_string_literal(int64_t start) {
  ScanError err;
  std::string_view text(iterator.data(), iterator.size());
  err.message = mark_text_with_message_and_context(text, start, start, 100, "Unterminated string literal.");
  return make_error<ScanError, Token>(std::move(err));
}

Token Scanner::identifier_or_keyword_token() {
  const int64_t start = iterator.next_index();
  const int64_t prev_was_period = iterator.peek_previous() == '.';  //  s.global, s.persistent

  while (iterator.has_next() && is_identifier_component(iterator.peek())) {
    iterator.advance();
  }

  auto lexeme = make_lexeme(start, iterator.next_index() - start);
  auto type = TokenType::identifier;

  if (!prev_was_period && matlab::is_keyword(lexeme)) {
    type = from_symbol(lexeme);
  }

  return {type, lexeme};
}

Token Scanner::number_literal_token() {
  const int64_t start = iterator.next_index();
  bool dot = false;

  while (iterator.has_next()) {
    auto c = iterator.peek();

    if (c == '.' && !dot) {
      dot = true;
    } else if (!is_digit(c)) {
      break;
    }

    iterator.advance();
  }

  if (iterator.peek() == 'e' && is_digit(iterator.peek_next())) {
    iterator.advance();

    while (iterator.has_next() && is_digit(iterator.peek())) {
      iterator.advance();
    }
  }

  return {TokenType::number_literal, make_lexeme(start, iterator.next_index() - start)};
}

}