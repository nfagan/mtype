#pragma once

#include "character.hpp"

namespace mt {

inline bool is_grouping_component(const Character& c) {
  return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']';
}

inline bool is_terminal_grouping_component(const Character& c) {
  return c == ')' || c == '}' || c == ']';
}

inline bool is_whitespace(const Character& c) {
  return c.is_ascii() && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

inline bool is_whitespace_excluding_new_line(const Character& c) {
  return c.is_ascii() && (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f');
}

inline bool is_digit(const Character& c) {
  return c.is_ascii() && c >= '0' && c <= '9';
}

inline bool is_alpha(const Character& c) {
  return c.is_ascii() && ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

inline bool is_alpha_numeric(const Character& c) {
  return is_digit(c) || is_alpha(c);
}

inline bool is_identifier_component(const Character& c) {
  return is_alpha_numeric(c) || c == '_';
}

inline bool can_precede_apostrophe(const Character& c) {
  return c == '\'' || c == '.' || is_digit(c) || is_identifier_component(c) || is_terminal_grouping_component(c);
}

}