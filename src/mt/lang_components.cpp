#include "lang_components.hpp"
#include <cassert>

namespace mt {

int precedence(BinaryOperator op) {
  switch (op) {
    case BinaryOperator::or_or:
      return 0;
    case BinaryOperator::and_and:
      return 1;
    case BinaryOperator::op_or:
      return 2;
    case BinaryOperator::op_and:
      return 3;
    case BinaryOperator::less:
    case BinaryOperator::less_equal:
    case BinaryOperator::greater:
    case BinaryOperator::greater_equal:
    case BinaryOperator::equal_equal:
    case BinaryOperator::not_equal:
      return 4;
    case BinaryOperator::colon:
      return 5;
    case BinaryOperator::plus:
    case BinaryOperator::minus:
      return 6;
    case BinaryOperator::times:
    case BinaryOperator::matrix_times:
    case BinaryOperator::left_divide:
    case BinaryOperator::matrix_left_divide:
    case BinaryOperator::right_divide:
    case BinaryOperator::matrix_right_divide:
      return 7;
    case BinaryOperator::power:
    case BinaryOperator::matrix_power:
      return 8;
    default:
      return -1;
  }
}

OperatorFixity fixity(UnaryOperator op) {
  switch (op) {
    case UnaryOperator::unary_plus:
    case UnaryOperator::unary_minus:
    case UnaryOperator::op_not:
      return OperatorFixity::pre;
    case UnaryOperator::transpose:
    case UnaryOperator::conjugate_transpose:
      return OperatorFixity::post;
    default:
      return OperatorFixity::pre;
  }
}

OperatorFixity fixity(BinaryOperator) {
  return OperatorFixity::in;
}

VariableDeclarationQualifier variable_declaration_qualifier_from_token_type(TokenType type) {
  switch (type) {
    case TokenType::keyword_persistent:
      return VariableDeclarationQualifier::persistent;
    case TokenType::keyword_global:
      return VariableDeclarationQualifier::global;
    default:
      assert(false && "No such variable declaration qualifier for type.");
      return VariableDeclarationQualifier::unknown;
  }
}

ControlFlowManipulator control_flow_manipulator_from_token_type(TokenType type) {
  switch (type) {
    case TokenType::keyword_return:
      return ControlFlowManipulator::keyword_return;
    case TokenType::keyword_break:
      return ControlFlowManipulator::keyword_break;
    case TokenType::keyword_continue:
      return ControlFlowManipulator::keyword_continue;
    default:
      assert(false && "No such control flow manipulator for type.");
      return ControlFlowManipulator::keyword_return;
  }
}

SubscriptMethod subscript_method_from_token_type(TokenType type) {
  switch (type) {
    case TokenType::left_parens:
      return SubscriptMethod::parens;
    case TokenType::left_brace:
      return SubscriptMethod::brace;
    case TokenType::period:
      return SubscriptMethod::period;
    default:
      assert(false && "No such subscript method for type.");
      return SubscriptMethod::unknown;
  }
}

GroupingMethod grouping_method_from_token_type(TokenType type) {
  switch (type) {
    case TokenType::left_parens:
      return GroupingMethod::parens;
    case TokenType::left_bracket:
      return GroupingMethod::bracket;
    case TokenType::left_brace:
      return GroupingMethod::brace;
    default:
      assert(false && "No such grouping method for type.");
      return GroupingMethod::unknown;
  }
}

UnaryOperator unary_operator_from_token_type(TokenType type) {
  switch (type) {
    case TokenType::plus:
      return UnaryOperator::unary_plus;
    case TokenType::minus:
      return UnaryOperator::unary_minus;
    case TokenType::tilde:
      return UnaryOperator::op_not;
    case TokenType::dot_apostrophe:
      return UnaryOperator::transpose;
    case TokenType::apostrophe:
      return UnaryOperator::conjugate_transpose;
    default:
      return UnaryOperator::unknown;
  }
}

BinaryOperator binary_operator_from_token_type(TokenType type) {
  switch (type) {
    case TokenType::plus:
      return BinaryOperator::plus;
    case TokenType::minus:
      return BinaryOperator::minus;
    case TokenType::asterisk:
      return BinaryOperator::matrix_times;
    case TokenType::forward_slash:
      return BinaryOperator::matrix_right_divide;
    case TokenType::back_slash:
      return BinaryOperator::matrix_left_divide;
    case TokenType::dot_asterisk:
      return BinaryOperator::times;
    case TokenType::dot_forward_slash:
      return BinaryOperator::right_divide;
    case TokenType::dot_back_slash:
      return BinaryOperator::left_divide;
    case TokenType::carat:
      return BinaryOperator::matrix_power;
    case TokenType::dot_carat:
      return BinaryOperator::power;
    case TokenType::ampersand:
      return BinaryOperator::op_and;
    case TokenType::double_ampersand:
      return BinaryOperator::and_and;
    case TokenType::vertical_bar:
      return BinaryOperator::op_or;
    case TokenType::double_vertical_bar:
      return BinaryOperator::or_or;
    case TokenType::less:
      return BinaryOperator::less;
    case TokenType::less_equal:
      return BinaryOperator::less_equal;
    case TokenType::greater:
      return BinaryOperator::greater;
    case TokenType::greater_equal:
      return BinaryOperator::greater_equal;
    case TokenType::not_equal:
      return BinaryOperator::not_equal;
    case TokenType::equal_equal:
      return BinaryOperator::equal_equal;
    case TokenType::colon:
      return BinaryOperator::colon;
    default:
      return BinaryOperator::unknown;
  }
}

}
