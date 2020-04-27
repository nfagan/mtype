#pragma once

#include "../error.hpp"
#include "../Optional.hpp"

namespace mt {
Optional<ParseError> insert_implicit_expr_delimiters(std::vector<Token>& tokens, std::string_view text);
void insert_implicit_expr_delimiters_in_if_condition(std::vector<Token>& tokens);
}