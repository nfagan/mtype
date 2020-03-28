#include "token_type.hpp"
#include <string>
#include <unordered_map>
#include <cassert>
#include <cstring>

namespace mt {

TokenType from_symbol(std::string_view s) {
  static std::unordered_map<std::string, TokenType> symbol_map{
    {"(", TokenType::left_parens},
    {")", TokenType::right_parens},
    {"[", TokenType::left_bracket},
    {"]", TokenType::right_bracket},
    {"{", TokenType::left_brace},
    {"}", TokenType::right_brace},
    {"=", TokenType::equal},
    {"==", TokenType::equal_equal},
    {"~=", TokenType::not_equal},
    {"<", TokenType::less},
    {">", TokenType::greater},
    {"<=", TokenType::less_equal},
    {">=", TokenType::greater_equal},
    {"~", TokenType::tilde},
    {":", TokenType::colon},
    {";", TokenType::semicolon},
    {".", TokenType::period},
    {",", TokenType::comma},
    {"+", TokenType::plus},
    {"-", TokenType::minus},
    {"*", TokenType::asterisk},
    {"/", TokenType::forward_slash},
    {"\\", TokenType::back_slash},
    {"^", TokenType::carat},
    {".*", TokenType::dot_asterisk},
    {"./", TokenType::dot_forward_slash},
    {".\\", TokenType::dot_back_slash},
    {".^", TokenType::dot_carat},
    {"@", TokenType::at},
    {"\n", TokenType::new_line},
    {"?", TokenType::question},
    {"'", TokenType::apostrophe},
    {"\"", TokenType::quote},
    {"|", TokenType::vertical_bar},
    {"&", TokenType::ampersand},
    {"||", TokenType::double_vertical_bar},
    {"&&", TokenType::double_ampersand},
    {"break", TokenType::keyword_break},
    {"case", TokenType::keyword_case},
    {"catch", TokenType::keyword_catch},
    {"classdef", TokenType::keyword_classdef},
    {"continue", TokenType::keyword_continue},
    {"else", TokenType::keyword_else},
    {"elseif", TokenType::keyword_elseif},
    {"end", TokenType::keyword_end},
    {"for", TokenType::keyword_for},
    {"function", TokenType::keyword_function},
    {"fun", TokenType::keyword_fun_type},
    {"global", TokenType::keyword_global},
    {"if", TokenType::keyword_if},
    {"otherwise", TokenType::keyword_otherwise},
    {"parfor", TokenType::keyword_parfor},
    {"persistent", TokenType::keyword_persistent},
    {"return", TokenType::keyword_return},
    {"spmd", TokenType::keyword_spmd},
    {"switch", TokenType::keyword_switch},
    {"try", TokenType::keyword_try},
    {"while", TokenType::keyword_while},
    {"enumeration", TokenType::keyword_enumeration},
    {"events", TokenType::keyword_events},
    {"methods", TokenType::keyword_methods},
    {"properties", TokenType::keyword_properties},
    {"import", TokenType::keyword_import},
    {"begin", TokenType::keyword_begin},
    {"export", TokenType::keyword_export},
    {"given", TokenType::keyword_given},
    {"let", TokenType::keyword_let},
    {"namespace", TokenType::keyword_namespace},
    {"struct", TokenType::keyword_struct},
    {"@T", TokenType::type_annotation_macro}
  };

  const auto it = symbol_map.find(std::string(s));

  if (it == symbol_map.end()) {
    return TokenType::null;
  } else {
    return it->second;
  }
}

const char* to_symbol(TokenType type) {
  switch (type) {
    case TokenType::left_parens:
      return "(";
    case TokenType::right_parens:
      return ")";
    case TokenType::left_brace:
      return "{";
    case TokenType::right_brace:
      return "}";
    case TokenType::left_bracket:
      return "[";
    case TokenType::right_bracket:
      return "]";
      //  Punct
    case TokenType::ellipsis:
      return "...";
    case TokenType::equal:
      return "=";
    case TokenType::equal_equal:
      return "==";
    case TokenType::not_equal:
      return "~=";
    case TokenType::less:
      return "<";
    case TokenType::greater:
      return ">";
    case TokenType::less_equal:
      return "<=";
    case TokenType::greater_equal:
      return ">=";
    case TokenType::tilde:
      return "~";
    case TokenType::colon:
      return ":";
    case TokenType::semicolon:
      return ";";
    case TokenType::period:
      return ".";
    case TokenType::comma:
      return ",";
    case TokenType::plus:
      return "+";
    case TokenType::minus:
      return "-";
    case TokenType::asterisk:
      return "*";
    case TokenType::forward_slash:
      return "/";
    case TokenType::back_slash:
      return "\\";
    case TokenType::carat:
      return "^";
    case TokenType::dot_asterisk:
      return ".*";
    case TokenType::dot_forward_slash:
      return "./";
    case TokenType::dot_back_slash:
      return ".\\";
    case TokenType::dot_carat:
      return ".^";
    case TokenType::dot_apostrophe:
      return ".'";
    case TokenType::at:
      return "@";
    case TokenType::new_line:
      return "\n";
    case TokenType::question:
      return "?";
    case TokenType::apostrophe:
      return "'";
    case TokenType::quote:
      return "\"";
    case TokenType::vertical_bar:
      return "|";
    case TokenType::ampersand:
      return "&";
    case TokenType::double_vertical_bar:
      return "||";
    case TokenType::double_ampersand:
      return "&&";
    case TokenType::op_end:
      return "end";
      //  Ident + literals
    case TokenType::identifier:
      return "<identifier>";
    case TokenType::char_literal:
      return "<char_literal>";
    case TokenType::string_literal:
      return "<string_literal>";
    case TokenType::number_literal:
      return "<number_literal>";
      //  Keywords
    case TokenType::keyword_break:
      return "break";
    case TokenType::keyword_case:
      return "case";
    case TokenType::keyword_catch:
      return "catch";
    case TokenType::keyword_classdef:
      return "classdef";
    case TokenType::keyword_continue:
      return "continue";
    case TokenType::keyword_else:
      return "else";
    case TokenType::keyword_elseif:
      return "elseif";
    case TokenType::keyword_end:
      return "end";
    case TokenType::keyword_for:
      return "for";
    case TokenType::keyword_function:
      return "function";
    case TokenType::keyword_global:
      return "global";
    case TokenType::keyword_if:
      return "if";
    case TokenType::keyword_otherwise:
      return "otherwise";
    case TokenType::keyword_parfor:
      return "parfor";
    case TokenType::keyword_persistent:
      return "persistent";
    case TokenType::keyword_return:
      return "return";
    case TokenType::keyword_spmd:
      return "spmd";
    case TokenType::keyword_switch:
      return "switch";
    case TokenType::keyword_try:
      return "try";
    case TokenType::keyword_while:
      return "while";
      //  Class keywords
    case TokenType::keyword_enumeration:
      return "enumeration";
    case TokenType::keyword_events:
      return "events";
    case TokenType::keyword_methods:
      return "methods";
    case TokenType::keyword_properties:
      return "properties";
    case TokenType::keyword_import:
      return "import";
      //  Typing keywords
    case TokenType::keyword_begin:
      return "begin";
    case TokenType::keyword_export:
      return "export";
    case TokenType::keyword_given:
      return "given";
    case TokenType::keyword_let:
      return "let";
    case TokenType::keyword_namespace:
      return "namespace";
    case TokenType::keyword_struct:
      return "struct";
    case TokenType::keyword_function_type:
      return "function";
    case TokenType::keyword_fun_type:
      return "fun";
    case TokenType::keyword_end_type:
      return "end";
    case TokenType::type_annotation_macro:
      return "@T";
    case TokenType::null:
      return "<null>";
  }
}

const char* to_string(TokenType type) {
  switch (type) {
    case TokenType::left_parens:
      return "left_parens";
    case TokenType::right_parens:
      return "right_parens";
    case TokenType::left_brace:
      return "left_brace";
    case TokenType::right_brace:
      return "right_brace";
    case TokenType::left_bracket:
      return "left_bracket";
    case TokenType::right_bracket:
      return "right_bracket";
    //  Punct
    case TokenType::ellipsis:
      return "ellipsis";
    case TokenType::equal:
      return "equal";
    case TokenType::equal_equal:
      return "equal_equal";
    case TokenType::not_equal:
      return "not_equal";
    case TokenType::less:
      return "less";
    case TokenType::greater:
      return "greater";
    case TokenType::less_equal:
      return "less_equal";
    case TokenType::greater_equal:
      return "greater_equal";
    case TokenType::tilde:
      return "tilde";
    case TokenType::colon:
      return "colon";
    case TokenType::semicolon:
      return "semicolon";
    case TokenType::period:
      return "period";
    case TokenType::comma:
      return "comma";
    case TokenType::plus:
      return "plus";
    case TokenType::minus:
      return "minus";
    case TokenType::asterisk:
      return "asterisk";
    case TokenType::forward_slash:
      return "forward_slash";
    case TokenType::back_slash:
      return "back_slash";
    case TokenType::carat:
      return "carat";
    case TokenType::dot_apostrophe:
      return "dot_apostrophe";
    case TokenType::dot_asterisk:
      return "dot_asterisk";
    case TokenType::dot_forward_slash:
      return "dot_forward_slash";
    case TokenType::dot_back_slash:
      return "dot_backslash";
    case TokenType::dot_carat:
      return "dot_carat";
    case TokenType::at:
      return "at";
    case TokenType::new_line:
      return "new_line";
    case TokenType::question:
      return "question";
    case TokenType::apostrophe:
      return "apostrophe";
    case TokenType::quote:
      return "quote";
    case TokenType::vertical_bar:
      return "vertical_bar";
    case TokenType::ampersand:
      return "ampersand";
    case TokenType::double_vertical_bar:
      return "double_vertical_bar";
    case TokenType::double_ampersand:
      return "double_ampersand";
    case TokenType::op_end:
      return "op_end";
    //  Ident + literals
    case TokenType::identifier:
      return "identifier";
    case TokenType::char_literal:
      return "char_literal";
    case TokenType::string_literal:
      return "string_literal";
    case TokenType::number_literal:
      return "number_literal";
    //  Keywords
    case TokenType::keyword_break:
      return "keyword_break";
    case TokenType::keyword_case:
      return "keyword_case";
    case TokenType::keyword_catch:
      return "keyword_catch";
    case TokenType::keyword_classdef:
      return "keyword_classdef";
    case TokenType::keyword_continue:
      return "keyword_continue";
    case TokenType::keyword_else:
      return "keyword_else";
    case TokenType::keyword_elseif:
      return "keyword_elseif";
    case TokenType::keyword_end:
      return "keyword_end";
    case TokenType::keyword_for:
      return "keyword_for";
    case TokenType::keyword_function:
      return "keyword_function";
    case TokenType::keyword_global:
      return "keyword_global";
    case TokenType::keyword_if:
      return "keyword_if";
    case TokenType::keyword_otherwise:
      return "keyword_otherwise";
    case TokenType::keyword_parfor:
      return "keyword_parfor";
    case TokenType::keyword_persistent:
      return "keyword_persistent";
    case TokenType::keyword_return:
      return "keyword_return";
    case TokenType::keyword_spmd:
      return "keyword_spmd";
    case TokenType::keyword_switch:
      return "keyword_switch";
    case TokenType::keyword_try:
      return "keyword_try";
    case TokenType::keyword_while:
      return "keyword_while";
    //  Class keywords
    case TokenType::keyword_enumeration:
      return "keyword_enumeration";
    case TokenType::keyword_events:
      return "keyword_events";
    case TokenType::keyword_methods:
      return "keyword_methods";
    case TokenType::keyword_properties:
      return "keyword_properties";
    case TokenType::keyword_import:
      return "keyword_import";
    //  Typing keywords
    case TokenType::keyword_begin:
      return "keyword_begin";
    case TokenType::keyword_export:
      return "keyword_export";
    case TokenType::keyword_given:
      return "keyword_given";
    case TokenType::keyword_let:
      return "keyword_let";
    case TokenType::keyword_namespace:
      return "keyword_namespace";
    case TokenType::keyword_struct:
      return "keyword_struct";
    case TokenType::keyword_function_type:
      return "keyword_function_type";
    case TokenType::keyword_fun_type:
      return "keyword_fun_type";
    case TokenType::keyword_end_type:
      return "keyword_end_type";
    case TokenType::type_annotation_macro:
      return "type_annotation_macro";
    case TokenType::null:
      return "null";
  }
}

bool unsafe_represents_keyword(TokenType type) {
  //  @TODO: Implement this more carefully.
  //  std::strlen("keyword")
  return std::strncmp("keyword", to_string(type), 7) == 0;
}

bool represents_literal(TokenType type) {
  return type == TokenType::string_literal || type == TokenType::char_literal ||
    type == TokenType::number_literal;
}

bool represents_expr_terminator(TokenType type) {
  return represents_stmt_terminator(type) || represents_grouping_terminator(type) || type == TokenType::equal;
}

bool represents_stmt_terminator(TokenType type) {
  return type == TokenType::semicolon || type == TokenType::comma || type == TokenType::new_line ||
    type == TokenType::null;
}

bool represents_grouping_component(TokenType type) {
  const auto min = static_cast<unsigned int>(TokenType::left_parens);
  const auto max = static_cast<unsigned int>(TokenType::right_bracket);
  const auto t = static_cast<unsigned int>(type);
  return t >= min && t <= max;
}

bool represents_grouping_initiator(TokenType type) {
  return type == TokenType::left_parens || type == TokenType::left_brace ||
    type == TokenType::left_bracket;
}

bool represents_grouping_terminator(TokenType type) {
  return type == TokenType::right_parens || type == TokenType::right_brace ||
    type == TokenType::right_bracket;
}

bool represents_binary_operator(TokenType type) {
  //  @Hack, first 21 token types are binary operators.
  return static_cast<unsigned int>(type) < 21;
}

bool represents_unary_operator(TokenType type) {
  return represents_postfix_unary_operator(type) || represents_prefix_unary_operator(type);
}

bool represents_prefix_unary_operator(TokenType type) {
  return type == TokenType::plus || type == TokenType::minus || type == TokenType::tilde;
}

bool can_precede_prefix_unary_operator(TokenType type) {
  switch (type) {
    case TokenType::identifier:
    case TokenType::string_literal:
    case TokenType::char_literal:
    case TokenType::number_literal:
    case TokenType::keyword_end:
    case TokenType::op_end:
      return false;
    default:
      return !represents_grouping_terminator(type) && !represents_postfix_unary_operator(type);
  }
}

bool can_be_skipped_in_classdef_block(TokenType type) {
  switch (type) {
    case TokenType::comma:
    case TokenType::semicolon:
    case TokenType::new_line:
      return true;
    default:
      return false;
  }
}

bool can_precede_postfix_unary_operator(TokenType type) {
  return represents_literal(type) || represents_grouping_terminator(type) ||
    type == TokenType::identifier || type == TokenType::apostrophe || type == TokenType::dot_apostrophe;
}

bool represents_postfix_unary_operator(TokenType type) {
  return type == TokenType::apostrophe || type == TokenType::dot_apostrophe;
}

std::array<TokenType, 3> grouping_terminators() {
  return {{TokenType::right_parens, TokenType::right_bracket, TokenType::right_bracket}};
}

TokenType grouping_terminator_for(TokenType initiator) {
  switch (initiator) {
    case TokenType::left_brace:
      return TokenType::right_brace;
    case TokenType::left_bracket:
      return TokenType::right_bracket;
    case TokenType::left_parens:
      return TokenType::right_parens;
    default:
      assert(false && "No grouping terminator for type.");
      return TokenType::null;
  }
}

}