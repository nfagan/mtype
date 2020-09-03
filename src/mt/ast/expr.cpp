#include "expr.hpp"
#include "def.hpp"
#include "StringVisitor.hpp"
#include "../parse/identifier_classification.hpp"
#include "visitor.hpp"
#include <cassert>
#include <algorithm>

namespace mt {

/*
 * PresumedSuperclassMethodReferenceExpr
 */

void SuperclassMethodReferenceExpr::accept(TypePreservingVisitor& visitor) {
  visitor.superclass_method_reference_expr(*this);
}

void SuperclassMethodReferenceExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.superclass_method_reference_expr(*this);
}

Expr* SuperclassMethodReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.superclass_method_reference_expr(*this);
}

std::string SuperclassMethodReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.superclass_method_reference_expr(*this);
}

/*
 * AnonymousFunctionExpr
 */

void AnonymousFunctionExpr::accept(TypePreservingVisitor& visitor) {
  visitor.anonymous_function_expr(*this);
}

void AnonymousFunctionExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.anonymous_function_expr(*this);
}

std::string AnonymousFunctionExpr::accept(const StringVisitor& vis) const {
  return vis.anonymous_function_expr(*this);
}

Expr* AnonymousFunctionExpr::accept(IdentifierClassifier& classifier) {
  return classifier.anonymous_function_expr(*this);
}

/*
 * FunctionReferenceExpr
 */

void FunctionReferenceExpr::accept(TypePreservingVisitor& visitor) {
  visitor.function_reference_expr(*this);
}

void FunctionReferenceExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.function_reference_expr(*this);
}

Expr* FunctionReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.function_reference_expr(*this);
}

std::string FunctionReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.function_reference_expr(*this);
}

/*
 * ColonSubscriptExpr
 */

void ColonSubscriptExpr::accept(TypePreservingVisitor& visitor) {
  visitor.colon_subscript_expr(*this);
}

void ColonSubscriptExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.colon_subscript_expr(*this);
}

std::string ColonSubscriptExpr::accept(const StringVisitor& vis) const {
  return vis.colon_subscript_expr(*this);
}

/*
 * CharLiteralExpr
 */

void CharLiteralExpr::accept(TypePreservingVisitor& visitor) {
  visitor.char_literal_expr(*this);
}

void CharLiteralExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.char_literal_expr(*this);
}

std::string CharLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.char_literal_expr(*this);
}

Optional<const CharLiteralExpr*> CharLiteralExpr::extract_char_literal_expr() const {
  return Optional<const CharLiteralExpr*>(this);
}

/*
 * StringLiteralExpr
 */

void StringLiteralExpr::accept(TypePreservingVisitor& visitor) {
  visitor.string_literal_expr(*this);
}

void StringLiteralExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.string_literal_expr(*this);
}

std::string StringLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.string_literal_expr(*this);
}

/*
 * NumberLiteralExpr
 */

void NumberLiteralExpr::accept(TypePreservingVisitor& visitor) {
  visitor.number_literal_expr(*this);
}

void NumberLiteralExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.number_literal_expr(*this);
}

std::string NumberLiteralExpr::accept(const StringVisitor& vis) const {
  return vis.number_literal_expr(*this);
}

/*
 * IgnoreFunctionArgumentExpr
 */

void IgnoreFunctionArgumentExpr::accept(TypePreservingVisitor& visitor) {
  visitor.ignore_function_argument_expr(*this);
}

void IgnoreFunctionArgumentExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.ignore_function_argument_expr(*this);
}

std::string IgnoreFunctionArgumentExpr::accept(const StringVisitor& vis) const {
  return vis.ignore_function_argument_expr(*this);
}

/*
 * DynamicFieldReferenceExpr
 */

void DynamicFieldReferenceExpr::accept(TypePreservingVisitor& visitor) {
  visitor.dynamic_field_reference_expr(*this);
}

void DynamicFieldReferenceExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.dynamic_field_reference_expr(*this);
}

std::string DynamicFieldReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.dynamic_field_reference_expr(*this);
}

Expr* DynamicFieldReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.dynamic_field_reference_expr(*this);
}

/*
 * LiteralFieldReferenceExpr
 */

void LiteralFieldReferenceExpr::accept(TypePreservingVisitor& visitor) {
  visitor.literal_field_reference_expr(*this);
}

void LiteralFieldReferenceExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.literal_field_reference_expr(*this);
}

std::string LiteralFieldReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.literal_field_reference_expr(*this);
}

/*
 * VariableReferenceExpr
 */

void VariableReferenceExpr::accept(TypePreservingVisitor& visitor) {
  visitor.variable_reference_expr(*this);
}

void VariableReferenceExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.variable_reference_expr(*this);
}

std::string VariableReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.variable_reference_expr(*this);
}

Optional<const VariableReferenceExpr*>
VariableReferenceExpr::extract_variable_reference_expr() const {
  return Optional<const VariableReferenceExpr*>(this);
}

/*
 * FunctionCallExpr
 */

void FunctionCallExpr::accept(TypePreservingVisitor& visitor) {
  visitor.function_call_expr(*this);
}

void FunctionCallExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.function_call_expr(*this);
}

std::string FunctionCallExpr::accept(const StringVisitor& vis) const {
  return vis.function_call_expr(*this);
}

Optional<const FunctionCallExpr*> FunctionCallExpr::extract_function_call_expr() const {
  return Optional<decltype(this)>(this);
}

/*
 * IdentifierReferenceExpr
 */

void IdentifierReferenceExpr::accept(TypePreservingVisitor& visitor) {
  visitor.identifier_reference_expr(*this);
}

void IdentifierReferenceExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.identifier_reference_expr(*this);
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
  identifier_components.push_back(primary_identifier.full_name());

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

bool IdentifierReferenceExpr::is_non_subscripted_scalar_reference() const {
  return subscripts.empty();
}

bool IdentifierReferenceExpr::is_maybe_non_subscripted_function_call() const {
  if (subscripts.size() > 1) {
    return false;
  } else if (subscripts.empty()) {
    return true;
  }

  return subscripts[0].method == SubscriptMethod::parens;
}

int64_t IdentifierReferenceExpr::num_primary_subscript_arguments() const {
  return subscripts.empty() ? 0 : subscripts[0].arguments.size();
}

Optional<const IdentifierReferenceExpr*> IdentifierReferenceExpr::extract_identifier_reference_expr() const {
  return Optional<const IdentifierReferenceExpr*>(this);
}

Optional<IdentifierReferenceExpr*> IdentifierReferenceExpr::extract_mut_identifier_reference_expr() {
  return Optional<IdentifierReferenceExpr*>(this);
}

std::string IdentifierReferenceExpr::accept(const StringVisitor& vis) const {
  return vis.identifier_reference_expr(*this);
}

Expr* IdentifierReferenceExpr::accept(IdentifierClassifier& classifier) {
  return classifier.identifier_reference_expr(*this);
}

/*
 * GroupingExpr
 */

void GroupingExpr::accept(TypePreservingVisitor& visitor) {
  visitor.grouping_expr(*this);
}

void GroupingExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.grouping_expr(*this);
}

bool GroupingExpr::is_brace() const {
  return method == GroupingMethod::brace;
}

bool GroupingExpr::is_bracket() const {
  return method == GroupingMethod::bracket;
}

bool GroupingExpr::is_parens() const {
  return method == GroupingMethod::parens;
}

bool GroupingExpr::is_valid_assignment_target() const {
  if (method != GroupingMethod::bracket || components.empty()) {
    return false;
  }

  return std::all_of(components.cbegin(), components.cend(), [](const auto& arg) {
    return arg.delimiter == TokenType::comma &&
      arg.expr->is_valid_assignment_target() &&
      !arg.expr->is_grouping_expr();
  });
}

Optional<const Expr*> GroupingExpr::extract_non_parens_grouping_sub_expr() const {
  if (!is_parens()) {
    return Optional<const Expr*>(this);

  } else if (components.size() == 1 &&
             components[0].expr->is_grouping_expr()) {
    const auto sub_expr = static_cast<const GroupingExpr*>(components[0].expr.get());
    return sub_expr->extract_non_parens_grouping_sub_expr();

  } else if (!components.empty()) {
    return Optional<const Expr*>(components[0].expr.get());

  } else {
    return NullOpt{};
  }
}

std::string GroupingExpr::accept(const StringVisitor& vis) const {
  return vis.grouping_expr(*this);
}

Expr* GroupingExpr::accept(IdentifierClassifier& classifier) {
  return classifier.grouping_expr(*this);
}

/*
 * EndOperatorExpr
 */

void EndOperatorExpr::accept(TypePreservingVisitor& visitor) {
  visitor.end_operator_expr(*this);
}

void EndOperatorExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.end_operator_expr(*this);
}

std::string EndOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.end_operator_expr(*this);
}

/*
 * UnaryOperatorExpr
 */

void UnaryOperatorExpr::accept(TypePreservingVisitor& visitor) {
  visitor.unary_operator_expr(*this);
}

void UnaryOperatorExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.unary_operator_expr(*this);
}

std::string UnaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.unary_operator_expr(*this);
}

Expr* UnaryOperatorExpr::accept(IdentifierClassifier& classifier) {
  return classifier.unary_operator_expr(*this);
}

/*
 * BinaryOperatorExpr
 */

void BinaryOperatorExpr::accept(TypePreservingVisitor& visitor) {
  visitor.binary_operator_expr(*this);
}

void BinaryOperatorExpr::accept_const(TypePreservingVisitor& visitor) const {
  visitor.binary_operator_expr(*this);
}

std::string BinaryOperatorExpr::accept(const StringVisitor& vis) const {
  return vis.binary_operator_expr(*this);
}

Expr* BinaryOperatorExpr::accept(IdentifierClassifier& classifier) {
  return classifier.binary_operator_expr(*this);
}

}