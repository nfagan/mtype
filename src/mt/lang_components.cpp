#include "lang_components.hpp"
#include "keyword.hpp"
#include "Optional.hpp"
#include "string.hpp"
#include <cassert>

namespace mt {

const char* to_symbol(ConcatenationDirection dir) {
  switch (dir) {
    case ConcatenationDirection::vertical:
      return "[;]";
    case ConcatenationDirection::horizontal:
      return "[,]";
    default:
      assert(false && "Unhandled");
      return "";
  }
}

const char* to_symbol(SubscriptMethod method) {
  switch (method) {
    case SubscriptMethod::parens:
      return "()";
    case SubscriptMethod::brace:
      return "{}";
    case SubscriptMethod::period:
      return ".";
    default:
      assert(false && "Unhandled");
      return "";
  }
}

const char* to_string(UnaryOperator op) {
  switch (op) {
    case UnaryOperator::unary_plus:
      return "unary_plus";
    case UnaryOperator::unary_minus:
      return "unary_minus";
    case UnaryOperator::op_not:
      return "op_not";
    case UnaryOperator::transpose:
      return "transpose";
    case UnaryOperator::conjugate_transpose:
      return "conjugate_transpose";
    default:
      assert(false && "Unhandled.");
      return "";
  }
}

const char* to_symbol(UnaryOperator op) {
  switch (op) {
    case UnaryOperator::unary_plus:
      return "+";
    case UnaryOperator::unary_minus:
      return "-";
    case UnaryOperator::op_not:
      return "~";
    case UnaryOperator::transpose:
      return ".'";
    case UnaryOperator::conjugate_transpose:
      return "'";
    default:
      assert(false && "Unhandled.");
      return "";
  }
}

const char* to_string(BinaryOperator op) {
  switch (op) {
    case BinaryOperator::plus:
      return "plus";
    case BinaryOperator::minus:
      return "minus";
    case BinaryOperator::times:
      return "times";
    case BinaryOperator::matrix_times:
      return "matrix_times";
    case BinaryOperator::right_divide:
      return "right_divide";
    case BinaryOperator::left_divide:
      return "left_divide";
    case BinaryOperator::matrix_right_divide:
      return "matrix_right_divide";
    case BinaryOperator::matrix_left_divide:
      return "matrix_left_divide";
    case BinaryOperator::power:
      return "power";
    case BinaryOperator::matrix_power:
      return "matrix_power";
    case BinaryOperator::op_and:
      return "op_and";
    case BinaryOperator::op_or:
      return "op_or";
    case BinaryOperator::and_and:
      return "and_and";
    case BinaryOperator::or_or:
      return "or_or";
    case BinaryOperator::less:
      return "less";
    case BinaryOperator::greater:
      return "greater";
    case BinaryOperator::less_equal:
      return "less_equal";
    case BinaryOperator::greater_equal:
      return "greater_equal";
    case BinaryOperator::equal_equal:
      return "equal_equal";
    case BinaryOperator::not_equal:
      return "not_equal";
    case BinaryOperator::colon:
      return "colon";
    default:
      assert(false && "Unhandled");
      return "";
  }
}

const char* to_symbol(BinaryOperator op) {
  switch (op) {
    case BinaryOperator::plus:
      return "+";
    case BinaryOperator::minus:
      return "-";
    case BinaryOperator::times:
      return ".*";
    case BinaryOperator::matrix_times:
      return "*";
    case BinaryOperator::right_divide:
      return "./";
    case BinaryOperator::left_divide:
      return ".\\";
    case BinaryOperator::matrix_right_divide:
      return "/";
    case BinaryOperator::matrix_left_divide:
      return "\\";
    case BinaryOperator::power:
      return ".^";
    case BinaryOperator::matrix_power:
      return "^";
    case BinaryOperator::op_and:
      return "&";
    case BinaryOperator::op_or:
      return "|";
    case BinaryOperator::and_and:
      return "&&";
    case BinaryOperator::or_or:
      return "||";
    case BinaryOperator::less:
      return "<";
    case BinaryOperator::greater:
      return ">";
    case BinaryOperator::less_equal:
      return "<=";
    case BinaryOperator::greater_equal:
      return ">=";
    case BinaryOperator::equal_equal:
      return "==";
    case BinaryOperator::not_equal:
      return "~=";
    case BinaryOperator::colon:
      return ":";
    default:
      assert(false && "Unhandled");
      return "";
  }
}

const char* to_string(IdentifierType type) {
  switch (type) {
    case IdentifierType::variable_assignment_or_initialization:
      return "variable_assignment_or_initialization";
    case IdentifierType::variable_reference:
      return "variable_reference";
    case IdentifierType::local_function:
      return "local_function";
    case IdentifierType::unresolved_external_function:
      return "unresolved_external_function";
    case IdentifierType::unknown:
      return "unknown";
    default:
      assert(false && "Unreachable.");
      return "unknown";
  }
}

bool is_variable(IdentifierType type) {
  switch (type) {
    case IdentifierType::variable_assignment_or_initialization:
    case IdentifierType::variable_reference:
      return true;
    default:
      return false;
  }
}

bool is_function(IdentifierType type) {
  switch (type) {
    case IdentifierType::unresolved_external_function:
    case IdentifierType::local_function:
      return true;
    default:
      return false;
  }
}

bool is_loop_control_flow_manipulator(ControlFlowManipulator manip) {
  return manip == ControlFlowManipulator::keyword_break || manip == ControlFlowManipulator::keyword_continue;
}

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
      assert(false && "Unhandled.");
      return UnaryOperator::unary_plus;
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

ConcatenationDirection concatenation_direction_from_token_type(TokenType type) {
  switch (type) {
    case TokenType::comma:
      return ConcatenationDirection::horizontal;
    case TokenType::semicolon:
      return ConcatenationDirection::vertical;
    default:
      assert(false && "Unhandled.");
      return ConcatenationDirection::horizontal;
  }
}

AccessType access_type_from_access_attribute_value(std::string_view value) {
  if (matlab::is_public_access_attribute_value(value)) {
    return AccessType::public_access;

  } else if (matlab::is_private_access_attribute_value(value)) {
    return AccessType::private_access;

  } else if (matlab::is_protected_access_attribute_value(value)) {
    return AccessType::protected_access;

  } else if (matlab::is_immutable_access_attribute_value(value)) {
    return AccessType::immutable_access;

  } else {
    assert(false && "Unknown access attribute value.");
    return AccessType::public_access;
  }
}


Optional<BinaryOperator> binary_operator_from_string(const std::string& str) {
  static std::unordered_map<std::string, BinaryOperator> op_map{
    {"plus", BinaryOperator::plus},
    {"minus", BinaryOperator::minus},
    {"times", BinaryOperator::times},
    {"mtimes", BinaryOperator::matrix_times},
    {"rdivide", BinaryOperator::right_divide},
    {"mrdivide", BinaryOperator::matrix_right_divide},
    {"ldivide", BinaryOperator::left_divide},
    {"mldivide", BinaryOperator::matrix_left_divide},
    {"power", BinaryOperator::power},
    {"mpower", BinaryOperator::matrix_power},
    {"lt", BinaryOperator::less},
    {"le", BinaryOperator::less_equal},
    {"gt", BinaryOperator::greater},
    {"ge", BinaryOperator::greater_equal},
    {"ne", BinaryOperator::not_equal},
    {"eq", BinaryOperator::equal_equal},
    {"and", BinaryOperator::op_and},
    {"or", BinaryOperator::op_or}
  };

  const auto it = op_map.find(str);
  return it == op_map.end() ? NullOpt{} : Optional<BinaryOperator>(it->second);
}

Optional<UnaryOperator> unary_operator_from_string(const std::string& str) {
  static std::unordered_map<std::string, UnaryOperator> op_map{
    {"uplus", UnaryOperator::unary_plus},
    {"uminus", UnaryOperator::unary_minus},
    {"not", UnaryOperator::op_not},
    {"ctranspose", UnaryOperator::conjugate_transpose},
    {"transpose", UnaryOperator::transpose},
  };

  const auto it = op_map.find(str);
  return it == op_map.end() ? NullOpt{} : Optional<UnaryOperator>(it->second);
}

bool represents_relation(BinaryOperator op) {
  switch (op) {
    case BinaryOperator::equal_equal:
    case BinaryOperator::not_equal:
    case BinaryOperator::less_equal:
    case BinaryOperator::less:
    case BinaryOperator::greater:
    case BinaryOperator::greater_equal:
      return true;
    default:
      return false;
  }
}

}
