#pragma once

#include <string_view>
#include <array>

namespace mt {

enum class TokenType : unsigned int {
  //  Binary operators
  equal_equal = 0,
  not_equal,
  less,
  greater,
  less_equal,
  greater_equal,
  plus,
  minus,
  asterisk,
  forward_slash,
  back_slash,
  carat,
  dot_asterisk,
  dot_forward_slash,
  dot_back_slash,
  dot_carat,
  vertical_bar,
  ampersand,
  double_vertical_bar,
  double_ampersand,
  colon,
  //  Unary operators
  tilde,
  apostrophe,
  dot_apostrophe,
  //  Grouping
  left_parens,
  right_parens,
  left_brace,
  right_brace,
  left_bracket,
  right_bracket,
  //  Punctuation
  ellipsis,
  equal,
  semicolon,
  period,
  comma,
  at,
  new_line,
  question,
  quote,
  //  Ident + literals
  identifier,
  char_literal,
  string_literal,
  number_literal,
  //  Keywords
  keyword_break,
  keyword_case,
  keyword_catch,
  keyword_classdef,
  keyword_continue,
  keyword_else,
  keyword_elseif,
  keyword_end,
  keyword_for,
  keyword_function,
  keyword_global,
  keyword_if,
  keyword_otherwise,
  keyword_parfor,
  keyword_persistent,
  keyword_return,
  keyword_spmd,
  keyword_switch,
  keyword_try,
  keyword_while,
  //  Class keywords
  keyword_enumeration,
  keyword_events,
  keyword_methods,
  keyword_properties,
  //  Contextual keywords
  keyword_import,
  //  Typing keywords
  keyword_begin,
  keyword_export,
  keyword_given,
  keyword_let,
  keyword_namespace,
  keyword_struct,
  keyword_function_type,
  keyword_end_type,
  //  Typing meta
  type_annotation,
  //  Meta
  null
};

const char* to_string(TokenType type);
const char* to_symbol(TokenType type);
TokenType from_symbol(std::string_view s);

bool represents_binary_operator(TokenType type);
bool represents_unary_operator(TokenType type);
bool represents_prefix_unary_operator(TokenType type);
bool represents_postfix_unary_operator(TokenType type);
bool represents_grouping_component(TokenType type);
bool represents_grouping_initiator(TokenType type);
bool represents_grouping_terminator(TokenType type);
bool represents_expr_terminator(TokenType type);
bool represents_literal(TokenType type);

TokenType grouping_terminator_for(TokenType initiator);
std::array<TokenType, 3> grouping_terminators();

}