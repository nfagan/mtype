#pragma once

#include "token_type.hpp"

namespace mt {

enum class IdentifierType : uint8_t {
  variable_reference,
  variable_assignment_or_initialization,
  resolved_local_function,
  unresolved_external_function,
  resolved_external_function,
  unknown
};

enum class VariableDeclarationQualifier : uint8_t {
  global,
  persistent,
  unknown
};

enum class ControlFlowManipulator : uint8_t {
  keyword_break,
  keyword_continue,
  keyword_return
};

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
ControlFlowManipulator control_flow_manipulator_from_token_type(TokenType type);
VariableDeclarationQualifier variable_declaration_qualifier_from_token_type(TokenType type);

bool is_loop_control_flow_manipulator(ControlFlowManipulator manip);

bool is_variable(IdentifierType type);
bool is_function(IdentifierType type);

OperatorFixity fixity(UnaryOperator op);
OperatorFixity fixity(BinaryOperator op);

int precedence(BinaryOperator op);

const char* to_string(IdentifierType type);

}