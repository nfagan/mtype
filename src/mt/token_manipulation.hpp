#pragma once

#include "error.hpp"
#include "Optional.hpp"

namespace mt {

Optional<ParseError> insert_implicit_expression_delimiters(std::vector<Token>& tokens,
                                                           std::string_view text);

}