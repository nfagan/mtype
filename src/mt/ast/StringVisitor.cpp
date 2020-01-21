#include "StringVisitor.hpp"
#include <algorithm>

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

std::string StringVisitor::for_stmt(const ForStmt& stmt) const {
  std::string str = tab_str() + "for " + std::string(stmt.loop_variable_identifier) + " = ";
  str += stmt.loop_variable_expr->accept(*this) + "\n";
  str += (stmt.body->accept(*this) + "\n" + tab_str() + "end");
  return str;
}

std::string StringVisitor::if_stmt(const IfStmt& stmt) const {
  auto str = if_branch(stmt.if_branch, "if");
  for (const auto& elseif_branch : stmt.elseif_branches) {
    //  @Note -- Directly pass branch to function here.
    str += ("\n" + if_branch(elseif_branch, "elseif"));
  }
  if (stmt.else_branch) {
    str += ("\n" + else_branch(stmt.else_branch.value()));
  }
  str += ("\n" + tab_str() + "end");
  return str;
}

std::string StringVisitor::while_stmt(const WhileStmt& stmt) const {
  auto str = tab_str() + "while " + stmt.condition_expr->accept(*this) + "\n";
  str += stmt.body->accept(*this);
  str += ("\n" + tab_str() + "end");
  return str;
}

std::string StringVisitor::switch_stmt(const SwitchStmt& stmt) const {
  auto str = tab_str() + "switch " + stmt.condition_expr->accept(*this) + "\n";
  enter_block();
  for (const auto& case_block : stmt.cases) {
    str += (tab_str() + "case " + case_block.expr->accept(*this) + "\n");
    str += (case_block.block->accept(*this) + "\n");
  }
  if (stmt.otherwise) {
    str += (tab_str() + "otherwise " + "\n" + stmt.otherwise->accept(*this) + "\n");
  }
  exit_block();
  str += (tab_str() + "end");
  return str;
}

std::string StringVisitor::try_stmt(const TryStmt& stmt) const {
  auto str = tab_str() + "try\n";
  str += (stmt.try_block->accept(*this) + "\n");

  if (stmt.catch_block) {
    str += (tab_str() + "catch ");
    if (stmt.catch_block.value().expr) {
      str += stmt.catch_block.value().expr->accept(*this);
    }
    str += ("\n" + stmt.catch_block.value().block->accept(*this) + "\n");
  }

  str += (tab_str() + "end");
  return str;
}

std::string StringVisitor::command_stmt(const CommandStmt& stmt) const {
  auto str = tab_str() + std::string(stmt.identifier_token.lexeme);
  for (const auto& arg : stmt.arguments) {
    str += (" " + std::string(arg.source_token.lexeme));
  }
  str += ";";
  return str;
}

std::string StringVisitor::variable_declaration_stmt(const VariableDeclarationStmt& stmt) const {
  //  @TODO: Derive declaration qualifier.
  return tab_str() + std::string(stmt.source_token.lexeme) + " " + join(stmt.identifiers, " ");
}

std::string StringVisitor::control_stmt(const ControlStmt& stmt) const {
  return tab_str() + std::string(stmt.source_token.lexeme);
}

std::string StringVisitor::if_branch(const IfBranch& branch, const char* branch_prefix) const {
  auto str = tab_str() + branch_prefix + " " + branch.condition_expr->accept(*this) + "\n";
  str += branch.block->accept(*this);
  return str;
}

std::string StringVisitor::else_branch(const ElseBranch& branch) const {
  return tab_str() + "else\n" + branch.block->accept(*this);
}

std::string StringVisitor::function_reference_expr(const FunctionReferenceExpr& expr) const {
  auto str = "@" + expr.expr->accept(*this);
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::anonymous_function_expr(const AnonymousFunctionExpr& expr) const {
  auto header = std::string("@(") + join(expr.input_identifiers, ", ") + ") ";
  auto body = expr.expr->accept(*this);
  auto str = header + body;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::colon_subscript_expr(const ColonSubscriptExpr&) const {
  return ":";
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

std::string StringVisitor::ignore_function_argument_expr(const IgnoreFunctionArgumentExpr&) const {
  return "~";
}

std::string StringVisitor::dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) const {
  auto str = expr.expr->accept(*this);
  return std::string("(") + str + ")";
}

std::string StringVisitor::literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) const {
  return std::string(expr.identifier_token.lexeme);
}

std::string StringVisitor::identifier_reference_expr(const IdentifierReferenceExpr& expr) const {
  const auto sub_str = visit_array(expr.subscripts, "");
  std::string str = std::string(expr.identifier_token.lexeme) + sub_str;
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
  std::vector<std::string> component_strs;
  std::transform(expr.components.cbegin(),
    expr.components.cend(), std::back_inserter(component_strs), [this](const auto& arg) {
    return grouping_expr_component(arg);
  });

  const auto sub_strs = join(component_strs, " ");
  const auto term = grouping_terminator_for(expr.source_token.type);

  const auto init_sym = to_symbol(expr.source_token.type);
  const auto term_symb = to_symbol(term);

  return std::string(init_sym) + sub_strs + std::string(term_symb);
}

std::string StringVisitor::unary_operator_expr(const UnaryOperatorExpr& expr) const {
  auto str = expr.expr->accept(*this);
  std::string op(to_symbol(expr.source_token.type));
  auto fix = fixity(expr.op);

  if (fix == OperatorFixity::pre) {
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

std::string StringVisitor::end_operator_expr(const EndOperatorExpr&) const {
  return "end";
}

std::string StringVisitor::scalar_type(const ScalarType& type) const {
  auto str = std::string(type.identifier);
  if (!type.arguments.empty()) {
    str += ("<" + visit_array(type.arguments, ", ") + ">");
  }
  return str;
}

std::string StringVisitor::function_type(const FunctionType& type) const {
  auto outputs = "[" + visit_array(type.outputs, ", ") + "]";
  auto inputs = "(" + visit_array(type.inputs, ", ") + ")";
  return outputs + " = " + inputs;
}

std::string StringVisitor::inline_type(const InlineType& type) const {
  return type.type->accept(*this);
}

std::string StringVisitor::type_given(const TypeGiven& given) const {
  return "given <" + join(given.identifiers, ", ") + "> " + given.declaration->accept(*this);
}

std::string StringVisitor::type_let(const TypeLet& let) const {
  return "let " + std::string(let.identifier) + " = " + let.equal_to_type->accept(*this);
}

std::string StringVisitor::type_begin(const TypeBegin& begin) const {
  std::string base_str = "begin";
  if (begin.is_exported) {
    base_str += " export";
  }
  base_str += "\n";
  enter_block();

  std::vector<std::string> lines;
  std::transform(begin.contents.cbegin(), begin.contents.cend(),
    std::back_inserter(lines), [this](const auto& annot) {
    return tab_str() + annot->accept(*this);
  });

  exit_block();
  auto contents_str = join(lines, "\n");
  auto end_str = "\n" + tab_str() + "end";
  return base_str + contents_str + end_str;
}

std::string StringVisitor::type_annot_macro(const TypeAnnotMacro& type) const {
  return tab_str() + "@t " + type.annotation->accept(*this);
}

}
