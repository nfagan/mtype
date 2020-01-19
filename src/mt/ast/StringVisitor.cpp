#include "StringVisitor.hpp"
#include <cmath>

namespace mt {

void StringVisitor::enter_block() const {
  tab_depth++;
}

void StringVisitor::exit_block() const {
  tab_depth--;
}

void StringVisitor::maybe_parenthesize(std::string& str) const {
  if (parenthesize_exprs) {
    str = "(" + str + ")";
  }
}

std::string StringVisitor::tab_str() const {
  std::string res;
  for (int i = 0; i < tab_depth; i++) {
    res += "  ";
  }
  return res;
}

std::string StringVisitor::block(const Block& block) const {
  enter_block();
  auto str = visit_array(block.nodes, "\n");
  exit_block();
  return str;
}

std::string StringVisitor::function_header(const FunctionHeader& header) const {
  auto outputs = join(header.outputs, ", ");
  auto inputs = join(header.inputs, ", ");
  std::string name(header.name);
  return "function [" + outputs + "]" + " = " + name + "(" + inputs + ")";
}

std::string StringVisitor::function_def(const FunctionDef& def) const {
  const auto header = tab_str() + function_header(def.header);
  auto body = def.body->accept(*this);
  auto end = tab_str() + "end";
  return header + "\n" + body + "\n" + end;
}

std::string StringVisitor::expr_stmt(const ExprStmt& stmt) const {
  return tab_str() + stmt.expr->accept(*this) + ";";
}

std::string StringVisitor::assignment_stmt(const AssignmentStmt& stmt) const {
  auto of = stmt.of_expr->accept(*this);
  auto to = stmt.to_expr->accept(*this);
  return tab_str() + of + " = " + to + ";";
}

std::string StringVisitor::char_literal_expr(const CharLiteralExpr& expr) const {
  return std::string("'") + std::string(expr.source_token.lexeme) + "'";
}

std::string StringVisitor::string_literal_expr(const StringLiteralExpr& expr) const {
  return std::string("\"") + std::string(expr.source_token.lexeme) + "\"";
}

std::string StringVisitor::number_literal_expr(const NumberLiteralExpr& expr) const {
  //  @Note: For debugging, show the value returned from stod rather than the lexeme.
  return std::to_string(expr.value);
}

std::string StringVisitor::ignore_function_output_argument_expr(const IgnoreFunctionOutputArgumentExpr&) const {
  return "~";
}

std::string StringVisitor::dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) const {
  auto str = expr.expr->accept(*this);
  return std::string("(") + str + ")";
}

std::string StringVisitor::literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) const {
  return std::string(expr.identifier.lexeme);
}

std::string StringVisitor::identifier_reference_expr(const IdentifierReferenceExpr& expr) const {
  const auto sub_str = visit_array(expr.subscripts, "");
  std::string str = std::string(expr.identifier.lexeme) + sub_str;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::subscript_expr(const SubscriptExpr& expr) const {
  auto str = visit_array(expr.arguments, ", ");

  if (expr.method == SubscriptMethod::parens) {
    str = "(" + str + ")";
  } else if (expr.method == SubscriptMethod::brace) {
    str = "{" + str + "}";
  } else if (expr.method == SubscriptMethod::period) {
    str = "." + str;
  }

  return str;
}

std::string StringVisitor::grouping_expr_component(const GroupingExprComponent& expr) const {
  return expr.expr->accept(*this) + to_symbol(expr.delimiter);
}

std::string StringVisitor::grouping_expr(const GroupingExpr& expr) const {
  const auto sub_strs = visit_array(expr.components, " ");
  const auto term = grouping_terminator_for(expr.source_token.type);

  const auto init_sym = to_symbol(expr.source_token.type);
  const auto term_symb = to_symbol(term);

  return std::string(init_sym) + sub_strs + std::string(term_symb);
}

std::string StringVisitor::unary_operator_expr(const UnaryOperatorExpr& expr) const {
  auto str = expr.expr->accept(*this);
  std::string op(to_symbol(expr.source_token.type));
  auto fixity = fixity_of_unary_operator(expr.op);

  if (fixity == OperatorFixity::pre) {
    std::swap(op, str);
  }

  str += op;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::binary_operator_expr(const BinaryOperatorExpr& expr) const {
  auto left = expr.left->accept(*this);
  auto right = expr.right->accept(*this);
  auto op = to_symbol(expr.source_token.type);

  std::string space = " ";
  auto str = left + space + op + space + right;
  maybe_parenthesize(str);

  return str;
}



}
