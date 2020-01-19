#include "expr.hpp"
#include "StringVisitor.hpp"

namespace mt {

std::string CharLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.char_literal_expr(*this);
}

std::string StringLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.string_literal_expr(*this);
}

std::string NumberLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.number_literal_expr(*this);
}

std::string IgnoreFunctionOutputArgumentExpr::accept(const StringVisitor& vis) const {
  return vis.ignore_function_output_argument_expr(*this);
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

std::string GroupingExprComponent::accept(const StringVisitor& vis) const {
  return vis.grouping_expr_component(*this);
}

std::string GroupingExpr::accept(const StringVisitor& vis) const {
  return vis.grouping_expr(*this);
}

std::string UnaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.unary_operator_expr(*this);
}

std::string BinaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.binary_operator_expr(*this);
}

}