#pragma once

#include "token_type.hpp"

namespace mt {

enum class SubscriptMethod : uint8_t {
  parens,
  brace,
  period,
  unknown
};

enum class GroupingMethod : uint8_t {
  parens,
  brace,
  bracket,
  unknown
};

enum class OperatorFixity : uint8_t {
  in,
  pre,
  post
};

enum class UnaryOperator : uint8_t {
  unary_plus = 0,
  unary_minus,
  op_not,
  transpose,
  conjugate_transpose,
  unknown
};

enum class BinaryOperator : unsigned int {
  plus = 0,
  minus,
  times,
  matrix_times,
  right_divide,
  left_divide,
  matrix_right_divide,
  matrix_left_divide,
  power,
  matrix_power,
  op_and,
  op_or,
  and_and,
  or_or,
  less,
  greater,
  less_equal,
  greater_equal,
  equal_equal,
  not_equal,
  colon,
  unknown
};

BinaryOperator binary_operator_from_token_type(TokenType type);
UnaryOperator unary_operator_from_token_type(TokenType type);
GroupingMethod grouping_method_from_token_type(TokenType type);
SubscriptMethod subscript_method_from_token_type(TokenType type);

OperatorFixity fixity_of_unary_operator(UnaryOperator op);
OperatorFixity fixity_of_binary_operator(UnaryOperator op);

}