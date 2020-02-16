#include "expr.hpp"
#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"
#include <cassert>

namespace mt {

Expr* PresumedSuperclassMethodReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.presumed_superclass_method_reference_expr(*this);
}

std::string PresumedSuperclassMethodReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.presumed_superclass_method_reference_expr(*this);
}

std::string AnonymousFunctionExpr::accept(const StringVisitor& vis) const {
  return vis.anonymous_function_expr(*this);
}

Expr* AnonymousFunctionExpr::accept(IdentifierClassifier& classifier) {
  return classifier.anonymous_function_expr(*this);
}

Expr* FunctionReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.function_reference_expr(*this);
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

Expr* DynamicFieldReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.dynamic_field_reference_expr(*this);
}

std::string LiteralFieldReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.literal_field_reference_expr(*this);
}

std::string VariableReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.variable_reference_expr(*this);
}

std::string FunctionCallExpr::accept(const StringVisitor& vis) const {
  return vis.function_call_expr(*this);
}

bool IdentifierReferenceExpr::is_static_identifier_reference_expr() const {
  for (const auto& subscript : subscripts) {
    const auto& args = subscript.arguments;

    if (subscript.method != SubscriptMethod::period ||
        args.size() != 1 ||
        !args[0]->is_literal_field_reference_expr()) {
      // a(1), a.('b'), a{1}
      return false;
    }
  }

  return true;
}

std::vector<int64_t> IdentifierReferenceExpr::make_compound_identifier(int64_t* end) const {
  std::vector<int64_t> identifier_components;
  identifier_components.push_back(primary_identifier);

  int64_t i = 0;
  const auto sz = int64_t(subscripts.size());

  while (i < sz && subscripts[i].method == SubscriptMethod::period) {
    const auto& sub = subscripts[i];
    assert(sub.arguments.size() == 1 && "Expected 1 argument for period subscript.");

    //  Period subscript expressions always have 1 argument.
    if (!sub.arguments[0]->append_to_compound_identifier(identifier_components)) {
      break;
    }

    i++;
  }

  *end = i;
  return identifier_components;
}

std::string IdentifierReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.identifier_reference_expr(*this);
}

Expr* IdentifierReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.identifier_reference_expr(*this);
}

std::string GroupingExpr::accept(const StringVisitor& vis) const {
  return vis.grouping_expr(*this);
}

Expr* GroupingExpr::accept(IdentifierClassifier& classifier) {
  return classifier.grouping_expr(*this);
}

std::string EndOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.end_operator_expr(*this);
}

std::string UnaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.unary_operator_expr(*this);
}

Expr* UnaryOperatorExpr::accept(IdentifierClassifier& classifier) {
  return classifier.unary_operator_expr(*this);
}

std::string BinaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.binary_operator_expr(*this);
}

Expr* BinaryOperatorExpr::accept(IdentifierClassifier& classifier) {
  return classifier.binary_operator_expr(*this);
}

}