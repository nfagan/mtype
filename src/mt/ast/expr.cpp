#include "expr.hpp"
#include "StringVisitor.hpp"

namespace mt {

std::string AnonymousFunctionExpr::accept(const StringVisitor& vis) const {
  return vis.anonymous_function_expr(*this);
}

std::string FunctionReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.function_reference_expr(*this);
}

std::string ColonSubscriptExpr::accept(const StringVisitor& vis) const {
  return vis.colon_subscript_expr(*this);
}

std::string CharLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.char_literal_expr(*this);
}

std::string StringLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.string_literal_expr(*this);
}

std::string NumberLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.number_literal_expr(*this);
}

std::string IgnoreFunctionArgumentExpr::accept(const StringVisitor& vis) const {
  return vis.ignore_function_argument_expr(*this);
}

std::string DynamicFieldReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.dynamic_field_reference_expr(*this);
}

std::string LiteralFieldReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.literal_field_reference_expr(*this);
}

std::string SubscriptExpr::accept(const StringVisitor& vis) const {
  return vis.subscript_expr(*this);
}

std::string IdentifierReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.identifier_reference_expr(*this);
}

std::string GroupingExpr::accept(const StringVisitor& vis) const {
  return vis.grouping_expr(*this);
}

std::string EndOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.end_operator_expr(*this);
}

std::string UnaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.unary_operator_expr(*this);
}

std::string BinaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.binary_operator_expr(*this);
}

}