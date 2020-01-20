#include "token_manipulation.hpp"
#include <cassert>

namespace mt {

namespace {

bool can_insert_comma_between(const Token& curr, const Token& next) {
  const bool first_is_lit_or_ident = represents_literal(curr.type) ||
      curr.type == TokenType::identifier;
  const bool first_is_term = represents_grouping_terminator(curr.type);
  const bool first_is_postfix = represents_postfix_unary_operator(curr.type);

  const bool can_maybe_insert = first_is_lit_or_ident || first_is_term || first_is_postfix;
  if (!can_maybe_insert) {
    return false;
  }

  const auto next_start = next.lexeme.data();
  const auto curr_stop = curr.lexeme.data() + curr.lexeme.size();
  const bool has_space_between = next_start - curr_stop >= 1;

  if (!has_space_between) {
    return false;
  }

  return represents_literal(next.type) || represents_grouping_initiator(next.type) ||
    next.type == TokenType::identifier;
}

}

Optional<ParseError> insert_implicit_expression_delimiters(std::vector<Token>& tokens,
                                                           std::string_view text) {
  TokenIterator it(&tokens);
  bool next_r_parens_is_anon_func_input_term = false;

  int bracket_depth = 0;
  int brace_depth = 0;

  std::vector<int64_t> insert_at_indices;

  while (it.has_next()) {
    const auto& curr = it.peek();
    const auto& next = it.peek_next();
//    const auto& prev = it.peek_prev();

    bool allow_r_parens_terminator = true;

    if (curr.type == TokenType::left_bracket) {
      bracket_depth++;
    } else if (curr.type == TokenType::right_bracket) {
      bracket_depth--;
    } else if (curr.type == TokenType::left_brace) {
      brace_depth++;
    } else if (curr.type == TokenType::right_brace) {
      brace_depth--;
    }

    if (brace_depth < 0 || bracket_depth < 0) {
      return Optional<ParseError>(ParseError(text, curr, "Unbalanced `{}` or `[]`."));
    }

    if (curr.type == TokenType::at && next.type == TokenType::left_parens) {
      next_r_parens_is_anon_func_input_term = true;

    } else if (next_r_parens_is_anon_func_input_term && curr.type == TokenType::right_parens) {
      next_r_parens_is_anon_func_input_term = false;
      allow_r_parens_terminator = false;
    }

    if ((bracket_depth > 0 || brace_depth > 0) && allow_r_parens_terminator) {
      if (can_insert_comma_between(curr, next)) {
        insert_at_indices.push_back(it.next_index());
      }
    }

    it.advance();
  }

  int64_t offset = 0;

  for (const auto& ind : insert_at_indices) {
    auto insert_at = ind + offset;
    assert(insert_at < int64_t(tokens.size()) && "Out of bounds read.");
    const auto& tok = tokens[insert_at];

    std::string_view space_lexeme(tok.lexeme.data() + tok.lexeme.size(), 1);
    Token comma{TokenType::comma, space_lexeme};

    tokens.insert(tokens.begin() + insert_at + 1, comma);
    offset++;
  }

  return NullOpt{};
}

}
