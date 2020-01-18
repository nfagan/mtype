#pragma once

#include <string_view>

namespace mt {

enum class TokenType {
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
  equal_equal,
  not_equal,
  less,
  greater,
  less_equal,
  greater_equal,
  tilde,
  colon,
  semicolon,
  period,
  comma,
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
  dot_apostrophe,
  at,
  new_line,
  question,
  apostrophe,
  quote,
  vertical_bar,
  ampersand,
  double_vertical_bar,
  double_ampersand,
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

}