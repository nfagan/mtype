#include "scan.hpp"
#include "character_traits.hpp"
#include "keyword.hpp"
#include "string.hpp"

namespace mt {

TokenType type_from_period(CharacterIterator& iter) {
  auto next = iter.peek();

  if (next == '*') {
    return TokenType::dot_asterisk;
  } else if (next == '/') {
    return TokenType::dot_forward_slash;
  } else if (next == '\\') {
    return TokenType::dot_back_slash;
  } else if (next == '^') {
    return TokenType::dot_carat;
  } else if (next == '\'') {
    return TokenType::dot_apostrophe;
  } else if (next == '.' && iter.peek_next() == '.') {
    return TokenType::ellipsis;
  } else {
    return TokenType::period;
  }
}

ScanResult Scanner::scan(const std::string& str) {
  return scan(str.c_str(), str.size());
}

ScanResult Scanner::scan(const char* text, int64_t len) {
  iterator = CharacterIterator(text, len);
  std::vector<Token> tokens;
  ScanErrors errors;
  new_line_is_type_annotation_terminator = false;
  block_comment_depth = 0;
  type_annotation_block_depth = 0;

  while (iterator.has_next()) {
    const auto c = iterator.peek();

    if (c == '%') {
      auto err = handle_comment(tokens);
      if (err) {
        errors.errors.emplace_back(err.rvalue());
      }

    } else if (block_comment_depth > 0 && !is_within_type_annotation()) {
      iterator.advance();

    } else if (is_alpha(c)) {
      auto token_result = identifier_or_keyword_token();
      check_add_token(token_result, errors, tokens);

    } else if (is_digit(c) || (c == '.' && is_digit(iterator.peek_next()))) {
      tokens.push_back(number_literal_token());

    } else if (c == '\n') {
      handle_new_line(tokens);

    } else if (c == '"') {
      auto token_result = string_literal_token(TokenType::string_literal, Character('"'));
      check_add_token(token_result, errors, tokens);

    } else if (c == '\'' && !can_precede_apostrophe(iterator.peek_previous())) {
      auto token_result = string_literal_token(TokenType::char_literal, Character('\''));
      check_add_token(token_result, errors, tokens);

    } else {
      handle_punctuation(tokens);

//      iterator.advance();
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

void Scanner::consume_to_new_line() {
  while (iterator.has_next() && iterator.peek() != '\n') {
    iterator.advance();
  }
}

void Scanner::consume_whitespace_to_new_line() {
  while (iterator.has_next() && is_whitespace_excluding_new_line(iterator.peek())) {
    iterator.advance();
  }
}

void Scanner::consume_whitespace() {
  while (iterator.has_next() && is_whitespace(iterator.peek())) {
    iterator.advance();
  }
}

void Scanner::check_add_token(mt::Result<mt::ScanError, mt::Token>& res,
                              ScanErrors& errs, std::vector<Token>& tokens) {
  if (res) {
    tokens.push_back(res.value);
  } else {
    errs.errors.push_back(res.error);
  }
}

bool Scanner::is_within_type_annotation() const {
  return new_line_is_type_annotation_terminator || type_annotation_block_depth > 0;
}

void Scanner::handle_punctuation(std::vector<Token>& tokens) {
  const int64_t start = iterator.next_index();
  const auto curr = iterator.advance();

  TokenType type = from_symbol(std::string_view(curr));
  if (type == TokenType::null) {
    return;
  }

  int size = 1;

  if (curr == '.') {
    type = type_from_period(iterator);

    if (type == TokenType::ellipsis) {
      iterator.advance(2);
      consume_whitespace_to_new_line();
      if (iterator.peek() == '\n') {
        iterator.advance();
      }
      //  Do not include ellipsis token.
      return;
    } else if (type != TokenType::period) {
      iterator.advance();
      size = 2;
    }
  } else if (curr == '>') {
    if (iterator.peek() == '=') {
      iterator.advance();
      type = TokenType::greater_equal;
      size = 2;
    }
  } else if (curr == '<') {
    if (iterator.peek() == '=') {
      iterator.advance();
      type = TokenType::less_equal;
      size = 2;
    }
  } else if (curr == '=') {
    if (iterator.peek() == '=') {
      iterator.advance();
      type = TokenType::equal_equal;
      size = 2;
    }
  } else if (curr == '~') {
    if (iterator.peek() == '=') {
      iterator.advance();
      type = TokenType::not_equal;
      size = 2;
    }
  } else if (curr == '&') {
    if (iterator.peek() == '&') {
      iterator.advance();
      type = TokenType::double_ampersand;
      size = 2;
    }
  } else if (curr == '|') {
    if (iterator.peek() == '|') {
      iterator.advance();
      type = TokenType::double_vertical_bar;
      size = 2;
    }
  }

  tokens.push_back({type, make_lexeme(start, size)});
}

void Scanner::handle_new_line(std::vector<Token>& tokens) {
  auto type = TokenType::new_line;

  if (new_line_is_type_annotation_terminator) {
    type = TokenType::keyword_end_type;
    new_line_is_type_annotation_terminator = false;
  }

  tokens.push_back({type, make_lexeme(iterator.next_index(), 1)});
  iterator.advance();
}

Optional<ScanError> Scanner::handle_comment(std::vector<Token>& tokens) {
  iterator.advance(); //  consume '%'

  const bool is_maybe_block_comment_start = iterator.peek() == '{';
  const bool is_maybe_block_comment_end = iterator.peek() == '}';
  const bool is_maybe_block_comment = is_maybe_block_comment_start || is_maybe_block_comment_end;

  if (is_maybe_block_comment) {
    iterator.advance();
  }

  consume_whitespace_to_new_line();

  const bool next_is_new_line = iterator.peek() == '\n';
  const bool is_block_comment_start = is_maybe_block_comment_start && next_is_new_line;
  const bool is_block_comment_end = is_maybe_block_comment_end && block_comment_depth > 0 &&
                                    (next_is_new_line || !iterator.has_next());

  if (is_block_comment_start) {
    block_comment_depth++;

  } else if (is_block_comment_end) {
    block_comment_depth--;
  }

  if (is_block_comment_start || is_block_comment_end) {
    consume_whitespace();
  }

  if (is_block_comment_end) {
    if (type_annotation_block_depth > 0) {
      auto err = make_error_at_start(iterator.next_index(),
        "Type annotations must be terminated before the end of a comment block.");
      return Optional<ScanError>(err);
    } else {
      return NullOpt{};
    }
  }

  const auto curr = iterator.peek();
  const auto next = iterator.peek_next();
  const auto next_next = iterator.peek_nth(2);

  const bool is_type_annotation = curr == '@' && next == 't' && is_whitespace(next_next);

  if (is_type_annotation && is_within_type_annotation()) {
    auto err = make_error_at_start(iterator.next_index(), "Type annotations cannot be nested in comments.");
    return Optional<ScanError>(err);
  }

  if (is_type_annotation) {
    handle_type_annotation_initializer(tokens, is_block_comment_start);

  } else if (!is_block_comment_start) {
    consume_to_new_line();

    if (iterator.peek() == '\n') {
      iterator.advance();
    }
  }

  return NullOpt{};
}

Optional<ScanError> Scanner::handle_type_annotation_initializer(std::vector<Token>& tokens, bool is_block_comment) {
  tokens.push_back({TokenType::type_annotation, make_lexeme(iterator.next_index(), 2)});
  iterator.advance(2);
  new_line_is_type_annotation_terminator = true;

  if (is_block_comment) {
    consume_whitespace_to_new_line();

    if (is_alpha(iterator.peek())) {
      //  @Hack -- Temporarily increase the block depth so that is_within_type_annotation() returns true.
      new_line_is_type_annotation_terminator = false;
      type_annotation_block_depth++;

      auto tok_result = identifier_or_keyword_token();
      if (!tok_result) {
        type_annotation_block_depth--;
        return Optional<ScanError>(tok_result.error);
      }

      type_annotation_block_depth--;
      new_line_is_type_annotation_terminator = tok_result.value.type != TokenType::keyword_begin;
      tokens.push_back(tok_result.value);
    }
  }

  return NullOpt{};
}

Result<ScanError, Token> Scanner::string_literal_token(mt::TokenType type, const mt::Character& terminator) {
  iterator.advance(); //  consume ' or "
  int64_t start = iterator.next_index();

  while (iterator.has_next()) {
    const auto c = iterator.peek();

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

ScanError Scanner::make_error_at_start(int64_t start, const char* message) const {
  std::string_view text(iterator.data() + start, iterator.size() - start);
  return ScanError(mark_text_with_message_and_context(text, start, start, 100, message));
}

Result<ScanError, Token> Scanner::identifier_or_keyword_token() {
  const int64_t start = iterator.next_index();
  const int64_t prev_was_period = iterator.peek_previous() == '.';  //  s.global, s.persistent

  while (iterator.has_next() && is_identifier_component(iterator.peek())) {
    iterator.advance();
  }

  const auto lexeme = make_lexeme(start, iterator.next_index() - start);
  auto type = TokenType::identifier;

  if (!prev_was_period) {
    const bool is_matlab_keyword = matlab::is_keyword(lexeme);
    const bool is_keyword = is_within_type_annotation() ? is_matlab_keyword || typing::is_keyword(lexeme) : is_matlab_keyword;

    if (is_keyword) {
      type = from_symbol(lexeme);

      if (is_within_type_annotation()) {
        if (lexeme == "end") {
          type_annotation_block_depth--;

          if (type_annotation_block_depth < 0) {
            auto err = make_error_at_start(start, "Unbalanced type annotation end.");
            return make_error<ScanError, Token>(err);
          }

          type = TokenType::keyword_end_type;
        } else if (mt::is_end_terminated(lexeme)) {
          if (new_line_is_type_annotation_terminator) {
            auto err = make_error_at_start(start, "Cannot introduce an end-terminated annotation here.");
            return make_error<ScanError, Token>(err);
          } else {
            type_annotation_block_depth++;
          }
        }
      }
    }
  }

  Token token{type, lexeme};
  return make_success<ScanError, Token>(token);
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