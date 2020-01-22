#include "scan.hpp"
#include "character_traits.hpp"
#include "keyword.hpp"
#include "string.hpp"

namespace mt {

bool EndTerminatedKeywordCounts::parent_is_classdef() const {
  return !keyword_types.empty() && keyword_types.back() == TokenType::keyword_classdef;
}

bool EndTerminatedKeywordCounts::is_non_end_terminated_function_file() const {
  if (keyword_counts.count(TokenType::keyword_function) == 0) {
    //  No functions in this file.
    return false;
  }

  if (keyword_types.empty()) {
    //  All keywords were terminated, leaving an empty stack.
    return false;
  }

  auto it = keyword_counts.cbegin();
  while (it != keyword_counts.cend()) {
    if (it->first != TokenType::keyword_function && it->second > 0) {
      //  A non-function-keyword block was left unterminated.
      return false;
    }

    it++;
  }

  //  Only function blocks were unterminated.
  return true;
}

Optional<ScanError> EndTerminatedKeywordCounts::register_keyword(TokenType keyword_type,
                                                                 bool requires_end,
                                                                 const CharacterIterator& iterator,
                                                                 int64_t start) {
  if (keyword_type == TokenType::keyword_end) {
    return pop_keyword(iterator, start);

  } else if (requires_end) {
    const auto it = keyword_counts.find(keyword_type);
    const int64_t count = it == keyword_counts.end() ? 0 : it->second;
    keyword_counts[keyword_type] = count + 1;
    keyword_types.push_back(keyword_type);
    return NullOpt{};

  } else {
    return NullOpt{};
  }
}

Optional<ScanError> EndTerminatedKeywordCounts::pop_keyword(const CharacterIterator& iterator, int64_t start) {
  if (keyword_types.empty()) {
    std::string_view text(iterator.data(), iterator.size());
    const char* message = "Missing `end` terminator.";
    auto err = mark_text_with_message_and_context(text, start, start, 100, message);
    return Optional<ScanError>(err);
  }

  const auto last_type = keyword_types.back();
  keyword_types.pop_back();
  keyword_counts[last_type] = keyword_counts[last_type] - 1;

  return NullOpt{};
}

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

ScanInfo Scanner::finalize_scan(std::vector<Token>& tokens) const {
  ScanInfo info(std::move(tokens));
  info.functions_are_end_terminated = !keyword_counts.is_non_end_terminated_function_file();

  return info;
}

void Scanner::begin_scan(const char* text, int64_t len) {
  iterator = CharacterIterator(text, len);
  source_text = std::string_view(text, len);

  new_line_is_type_annotation_terminator = false;
  block_comment_depth = 0;
  type_annotation_block_depth = 0;
  marked_unbalanced_grouping_character_error = false;

  parens_depth = 0;
  brace_depth = 0;
  bracket_depth = 0;

  keyword_counts = EndTerminatedKeywordCounts();
}

ScanResult Scanner::scan(const char* text, int64_t len) {
  begin_scan(text, len);

  std::vector<Token> tokens;
  ScanErrors errors;

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
      auto err = handle_punctuation(tokens);
      if (err) {
        errors.errors.emplace_back(err.rvalue());
      }
    }
  }

  tokens.push_back({TokenType::null, std::string_view()});

  if (errors.errors.empty()) {
    return make_success<ScanErrors, ScanInfo>(finalize_scan(tokens));

  } else {
    return make_error<ScanErrors, ScanInfo>(std::move(errors));
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

bool Scanner::is_scientific_notation_number_literal(int* num_to_advance) const {
  bool is_maybe_sci_not = iterator.peek() == 'e';
  const auto next = iterator.peek_next();
  const bool next_is_sign = next == '+' || next == '-';

  if (!is_maybe_sci_not) {
    return false;
  }

  if (next_is_sign) {
    //  1e-3 or 1e+3
    is_maybe_sci_not = is_digit(iterator.peek_nth(2));
    *num_to_advance = 2;
  } else  {
    //  1e3
    is_maybe_sci_not = is_digit(next);
    *num_to_advance = 1;
  }

  return true;
}

Optional<ScanError> Scanner::handle_punctuation(std::vector<Token>& tokens) {
  const int64_t start = iterator.next_index();
  const auto curr = iterator.advance();

  TokenType type = from_symbol(std::string_view(curr));
  if (type == TokenType::null) {
    return NullOpt{};
  }

  int size = 1;

  if (curr == '.') {
    type = type_from_period(iterator);

    if (type == TokenType::ellipsis) {
      iterator.advance(2);
      consume_to_new_line();
      if (iterator.peek() == '\n') {
        iterator.advance();
      }
      //  Do not include ellipsis token.
      return NullOpt{};
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

  auto grouping_character_err = update_grouping_character_depth(curr, start);
  if (grouping_character_err) {
    return grouping_character_err;
  } else {
    tokens.push_back({type, make_lexeme(start, size)});
    return NullOpt{};
  }
}

Optional<ScanError> Scanner::update_grouping_character_depth(const Character& c, int64_t start) {
  bool is_grouping_char = true;

  if (c == '(') {
    parens_depth++;
  } else if (c == ')') {
    parens_depth--;
  } else if (c == '[') {
    bracket_depth++;
  } else if (c == ']') {
    bracket_depth--;
  } else if (c == '{') {
    brace_depth++;
  } else if (c == '}') {
    brace_depth--;
  } else {
    is_grouping_char = false;
  }

  if (!is_grouping_char || marked_unbalanced_grouping_character_error) {
    return NullOpt{};
  }

  if (parens_depth < 0 || brace_depth < 0 || bracket_depth < 0) {
    marked_unbalanced_grouping_character_error = true;
    return Optional<ScanError>(make_error_unbalanced_grouping_character(start));
  } else {
    return NullOpt{};
  }
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

  const bool is_type_annotation = curr == '@' && next == 'T' && is_whitespace(next_next);

  if (is_type_annotation && is_within_type_annotation()) {
    auto err = make_error_at_start(iterator.next_index(),
      "Type annotations cannot be nested in comments.");
    return Optional<ScanError>(err);
  }

  if (is_type_annotation) {
    handle_type_annotation_initializer(tokens, is_block_comment_start);

  } else if (!is_block_comment_start) {
    //  @TODO: Only allow the insertion of a newline token if the comment was the first
    //  non-whitespace character on that line.
    consume_to_new_line();
  }

  return NullOpt{};
}

Optional<ScanError> Scanner::handle_type_annotation_initializer(std::vector<Token>& tokens, bool is_block_comment) {
  tokens.push_back({TokenType::type_annotation_macro, make_lexeme(iterator.next_index(), 2)});
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
        //  'a''' -> a'
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

Result<ScanError, Token> Scanner::identifier_or_keyword_token() {
  const int64_t start = iterator.next_index();
  const int64_t prev_was_period = iterator.peek_previous() == '.';  //  s.global, s.persistent

  while (iterator.has_next() && is_identifier_component(iterator.peek())) {
    iterator.advance();
  }

  const auto lexeme = make_lexeme(start, iterator.next_index() - start);
  auto type = TokenType::identifier;

  if (prev_was_period) {
    //  Always treat as an identifier.
    return make_success<ScanError, Token>(Token{type, lexeme});
  }

  const bool is_matlab_keyword = matlab::is_keyword(lexeme);
  bool is_keyword = is_matlab_keyword;

  if (is_within_type_annotation()) {
    is_keyword = is_keyword || typing::is_keyword(lexeme);

  } else if (keyword_counts.parent_is_classdef()) {
    is_keyword = is_keyword || matlab::is_classdef_keyword(lexeme);
  }

  if (!is_keyword) {
    return make_success<ScanError, Token>(Token{type, lexeme});
  }

  //  This is a keyword.
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

  if (type == TokenType::keyword_end && (brace_depth > 0 || parens_depth > 0)) {
    //  Recode (end) to op_end
    type = TokenType::op_end;
  }

  if (!is_within_type_annotation()) {
    bool requires_end = matlab::is_end_terminated(lexeme);
    auto maybe_err = keyword_counts.register_keyword(type, requires_end, iterator, start);
    if (maybe_err) {
      return make_error<ScanError, Token>(maybe_err.rvalue());
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

  int num_to_advance = 0;

  if (is_scientific_notation_number_literal(&num_to_advance)) {
    iterator.advance(num_to_advance);

    while (iterator.has_next() && is_digit(iterator.peek())) {
      iterator.advance();
    }
  }

  if (iterator.peek() == 'i' || iterator.peek() == 'j') {
    //  @Note: Complex numbers are not handled here.
    iterator.advance();
  }

  return {TokenType::number_literal, make_lexeme(start, iterator.next_index() - start)};
}

ScanError Scanner::make_error_unbalanced_grouping_character(int64_t start) const {
  return make_error_at_start(start, "Unbalanced `()`, `[]`, or `{}`.");
}

Result<ScanError, Token> Scanner::make_error_unterminated_string_literal(int64_t start) const {
  ScanError err;
  std::string_view text(iterator.data(), iterator.size());
  err.message = mark_text_with_message_and_context(text, start, start, 100, "Unterminated string literal.");
  return make_error<ScanError, Token>(std::move(err));
}

ScanError Scanner::make_error_at_start(int64_t start, const char* message) const {
  std::string_view text(iterator.data(), iterator.size());
  return ScanError(mark_text_with_message_and_context(text, start, start, 100, message));
}

}