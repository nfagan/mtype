#pragma once

#include "ast.hpp"
#include "def.hpp"
#include "stmt.hpp"
#include "expr.hpp"
#include "../string.hpp"

namespace mt {

class StringVisitor {
public:
  StringVisitor() : parenthesize_exprs(true), tab_depth(0) {
    //
  }
  ~StringVisitor() = default;

  std::string function_def(const FunctionDef& def) const;
  std::string function_header(const FunctionHeader& header) const;
  std::string block(const Block& block) const;

  std::string expr_stmt(const ExprStmt& stmt) const;
  std::string assignment_stmt(const AssignmentStmt& stmt) const;

  std::string char_literal_expr(const CharLiteralExpr& expr) const;
  std::string string_literal_expr(const StringLiteralExpr& expr) const;
  std::string number_literal_expr(const NumberLiteralExpr& expr) const;
  std::string ignore_function_output_argument_expr(const IgnoreFunctionOutputArgumentExpr& expr) const;
  std::string dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) const;
  std::string literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) const;
  std::string subscript_expr(const SubscriptExpr& expr) const;
  std::string identifier_reference_expr(const IdentifierReferenceExpr& expr) const;
  std::string grouping_expr_component(const GroupingExprComponent& expr) const;
  std::string grouping_expr(const GroupingExpr& expr) const;
  std::string unary_operator_expr(const UnaryOperatorExpr& expr) const;
  std::string binary_operator_expr(const BinaryOperatorExpr& expr) const;

private:
  void maybe_parenthesize(std::string& str) const;
  std::string tab_str() const;

  template <typename T>
  std::string visit_array(const std::vector<T>& visitables, const std::string& delim) const;

  void enter_block() const;
  void exit_block() const;

public:
  bool parenthesize_exprs;

private:
  mutable int tab_depth;
};

}

namespace detail {
  template <typename T>
  inline auto accept_impl(const mt::StringVisitor& vis, const std::unique_ptr<T>& arg) {
    return arg->accept(vis);
  }

  template <typename T>
  inline typename std::enable_if<mt::is_ast_node<T>::value, std::string>
  accept_impl(const mt::StringVisitor& vis, const T& arg) {
    return arg.accept(vis);
  }
}

template <typename T>
std::string mt::StringVisitor::visit_array(const std::vector<T>& visitables,
                                           const std::string& delim) const {
  std::vector<std::string> values;
  for (const auto& arg : visitables) {
    values.emplace_back(detail::accept_impl(*this, arg));
  }
  return mt::join(values, delim);
}