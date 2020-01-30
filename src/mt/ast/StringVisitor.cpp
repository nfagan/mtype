#include "StringVisitor.hpp"
#include <algorithm>
#include <cassert>

namespace mt {

namespace terminal_colors {
  const char* const red = "\x1B[31m";
  const char* const green = "\x1B[32m";
  const char* const yellow = "\x1B[33m";
  const char* const blue = "\x1B[34m";
  const char* const dflt = "\x1B[0m";
  std::array<const char*, 5> all = {{red, green, yellow, blue, dflt}};
}

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

void StringVisitor::maybe_colorize(std::string& str, TokenType type) const {
  using namespace terminal_colors;
  if (!colorize) {
    return;
  }

  const char* color = dflt;

  if (unsafe_represents_keyword(type)) {
    color = yellow;
  }

  str = color + str + dflt;
}

void StringVisitor::maybe_colorize(std::string& str, int color_code_index) const {
  using namespace terminal_colors;
  const char* color_code = dflt;
  if (color_code_index >= 0 && color_code_index < int(all.size())) {
    color_code = all[color_code_index];
  }
  maybe_colorize(str, color_code);
}

void StringVisitor::maybe_colorize(std::string& str, const char* color_code) const {
  if (colorize) {
    str = color_code + str + terminal_colors::dflt;
  }
}

std::string StringVisitor::tab_str() const {
  std::string res;
  for (int i = 0; i < tab_depth; i++) {
    res += "  ";
  }
  return res;
}

std::string StringVisitor::end_str() const {
  auto str = std::string("end");
  maybe_colorize(str, TokenType::keyword_end);
  return str;
}

std::string StringVisitor::subscripts(const std::vector<Subscript>& subs) const {
  std::string sub_str;
  for (const auto& sub : subs) {
    sub_str += subscript_expr(sub);
  }
  return sub_str;
}

std::string StringVisitor::root_block(const RootBlock& block) const {
  return block.block->accept(*this);
}

std::string StringVisitor::block(const Block& block) const {
  enter_block();
  auto str = visit_array(block.nodes, "\n");
  exit_block();
  return str;
}

std::string StringVisitor::function_header(const FunctionHeader& header) const {
  auto outputs = join(string_registry->collect(header.outputs), ", ");
  auto inputs = join(string_registry->collect(header.inputs), ", ");
  auto name = std::string(string_registry->at(header.name));
  auto func = std::string("function");
  maybe_colorize(func, TokenType::keyword_function);
  return func + " [" + outputs + "]" + " = " + name + "(" + inputs + ")";
}

std::string StringVisitor::function_reference(const FunctionReference& reference) const {
  std::string ptr_str;
  if (include_def_ptrs) {
    ptr_str = "<" + ptr_to_hex_string(&reference) + "> ";
  }
  return tab_str() + ptr_str + function_def(*reference.def);
}

std::string StringVisitor::function_def(const FunctionDef& def) const {
  const auto header = function_header(def.header);
  auto body = def.body->accept(*this);
  auto end = tab_str() + end_str();
  return header + "\n" + body + "\n" + end;
}

std::string StringVisitor::variable_def(const VariableDef& def) const {
  return tab_str() + std::string(string_registry->at(def.name));
}

std::string StringVisitor::expr_stmt(const ExprStmt& stmt) const {
  return tab_str() + stmt.expr->accept(*this) + ";";
}

std::string StringVisitor::assignment_stmt(const AssignmentStmt& stmt) const {
  auto of = stmt.of_expr->accept(*this);
  auto to = stmt.to_expr->accept(*this);
  return tab_str() + to + " = " + of + ";";
}

std::string StringVisitor::for_stmt(const ForStmt& stmt) const {
  std::string for_str = "for";
  maybe_colorize(for_str, stmt.source_token.type);
  std::string loop_var_str = std::string(string_registry->at(stmt.loop_variable_identifier));
  std::string str = tab_str() + for_str + " " + loop_var_str + " = ";
  str += stmt.loop_variable_expr->accept(*this) + "\n";
  str += (stmt.body->accept(*this) + "\n" + tab_str() + end_str());
  return str;
}

std::string StringVisitor::if_stmt(const IfStmt& stmt) const {
  auto str = if_branch(stmt.if_branch, "if");
  for (const auto& elseif_branch : stmt.elseif_branches) {
    str += ("\n" + if_branch(elseif_branch, "elseif"));
  }
  if (stmt.else_branch) {
    str += ("\n" + else_branch(stmt.else_branch.value()));
  }
  str += ("\n" + tab_str() + end_str());
  return str;
}

std::string StringVisitor::while_stmt(const WhileStmt& stmt) const {
  std::string while_str = "while ";
  maybe_colorize(while_str, stmt.source_token.type);
  auto str = tab_str() + while_str + stmt.condition_expr->accept(*this) + "\n";
  str += stmt.body->accept(*this);
  str += ("\n" + tab_str() + end_str());
  return str;
}

std::string StringVisitor::switch_stmt(const SwitchStmt& stmt) const {
  std::string switch_str = "switch ";
  maybe_colorize(switch_str, stmt.source_token.type);
  auto str = tab_str() + switch_str + stmt.condition_expr->accept(*this) + "\n";
  enter_block();
  for (const auto& case_block : stmt.cases) {
    std::string case_str = "case ";
    maybe_colorize(case_str, case_block.source_token.type);
    str += (tab_str() + case_str + case_block.expr->accept(*this) + "\n");
    str += (case_block.block->accept(*this) + "\n");
  }
  if (stmt.otherwise) {
    std::string otherwise_str = "otherwise ";
    maybe_colorize(otherwise_str, TokenType::keyword_otherwise);
    str += (tab_str() + otherwise_str + "\n" + stmt.otherwise->accept(*this) + "\n");
  }
  exit_block();
  str += (tab_str() + end_str());
  return str;
}

std::string StringVisitor::try_stmt(const TryStmt& stmt) const {
  std::string try_str = "try";
  maybe_colorize(try_str, stmt.source_token.type);

  auto str = tab_str() + try_str + "\n";
  str += (stmt.try_block->accept(*this) + "\n");

  if (stmt.catch_block) {
    std::string catch_str = "catch ";
    maybe_colorize(catch_str, stmt.catch_block.value().source_token.type);
    str += (tab_str() + catch_str);
    if (stmt.catch_block.value().expr) {
      str += stmt.catch_block.value().expr->accept(*this);
    }
    str += ("\n" + stmt.catch_block.value().block->accept(*this) + "\n");
  }

  str += (tab_str() + end_str());
  return str;
}

std::string StringVisitor::command_stmt(const CommandStmt& stmt) const {
  auto str = tab_str() + std::string(string_registry->at(stmt.command_identifier));
  for (const auto& arg : stmt.arguments) {
    str += (" " + std::string(arg.source_token.lexeme));
  }
  str += ";";
  return str;
}

std::string StringVisitor::variable_declaration_stmt(const VariableDeclarationStmt& stmt) const {
  //  @TODO: Derive declaration qualifier.
  auto identifier_strs = string_registry->collect(stmt.identifiers);
  return tab_str() + std::string(stmt.source_token.lexeme) + " " + join(identifier_strs, " ");
}

std::string StringVisitor::control_stmt(const ControlStmt& stmt) const {
  return tab_str() + std::string(stmt.source_token.lexeme);
}

std::string StringVisitor::if_branch(const IfBranch& branch, const char* branch_prefix) const {
  std::string branch_str(branch_prefix);
  maybe_colorize(branch_str, branch.source_token.type);
  auto str = tab_str() + branch_str + " " + branch.condition_expr->accept(*this) + "\n";
  str += branch.block->accept(*this);
  return str;
}

std::string StringVisitor::else_branch(const ElseBranch& branch) const {
  std::string else_str = "else";
  maybe_colorize(else_str, branch.source_token.type);
  return tab_str() + else_str + "\n" + branch.block->accept(*this);
}

std::string StringVisitor::function_reference_expr(const FunctionReferenceExpr& expr) const {
  auto str = "@" + join(string_registry->collect(expr.identifier_components), ".");
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::variable_reference_expr(const VariableReferenceExpr& expr) const {
  std::string str = std::string(string_registry->at(expr.name));
  std::string sub_str = subscripts(expr.subscripts);

  if (expr.is_initializer) {
    maybe_colorize(str, 3);
  }

  if (include_def_ptrs) {
    str = "<" + ptr_to_hex_string(expr.variable_def) + " " + str + ">";
  }

  str += sub_str;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::function_call_expr(const FunctionCallExpr& expr) const {
  auto name = std::string(string_registry->at(expr.function_reference->name));
  const auto arg_str = visit_array(expr.arguments, ", ");
  auto sub_str = subscripts(expr.subscripts);

  const std::string call_expr_str = "(" + arg_str + ")" + sub_str;

  if (include_identifier_classification) {
    auto* function_def = expr.function_reference->def;
    int color_code = function_def ? 1 : 0;
    maybe_colorize(name, color_code);

    if (include_def_ptrs) {
      auto ptr_str = ptr_to_hex_string(expr.function_reference) + " ";
      return "<" + ptr_str + name + ">" + call_expr_str;
    } else {
      return name + call_expr_str;
    }
  } else {
    return name + call_expr_str;
  }
}

std::string StringVisitor::anonymous_function_expr(const AnonymousFunctionExpr& expr) const {
  std::vector<std::string> identifier_strs;
  for (const auto& identifier : expr.input_identifiers) {
    if (identifier) {
      identifier_strs.emplace_back(string_registry->at(identifier.value()));
    } else {
      identifier_strs.emplace_back("~");
    }
  }
  auto header = std::string("@(") + join(identifier_strs, ", ") + ") ";
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
  return std::string(string_registry->at(expr.field_identifier));
}

std::string StringVisitor::identifier_reference_expr(const IdentifierReferenceExpr& expr) const {
  std::string sub_str = subscripts(expr.subscripts);
  std::string str = std::string(string_registry->at(expr.primary_identifier)) + sub_str;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::subscript_expr(const Subscript& expr) const {
  auto str = visit_array(expr.arguments, ", ");

  switch (expr.method) {
    case SubscriptMethod::parens:
      return "(" + str + ")";
    case SubscriptMethod::brace:
      return "{" + str + "}";
    case SubscriptMethod::period:
      return "." + str;
    default:
      assert(false && "Unhandled subscript method.");
      return str;
  }
}

std::string StringVisitor::grouping_expr_component(const GroupingExprComponent& expr, bool include_delimiter) const {
  auto str = expr.expr->accept(*this);
  if (include_delimiter) {
    str += to_symbol(expr.delimiter);
  }
  return str;
}

std::string StringVisitor::grouping_expr(const GroupingExpr& expr) const {
  std::vector<std::string> component_strs;

  for (int64_t i = 0; i < int64_t(expr.components.size()); i++) {
    const bool include_delim = i < int64_t(expr.components.size()) - 1;
    component_strs.emplace_back(grouping_expr_component(expr.components[i], include_delim));
  }

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

  auto str = left + " " + op + " " + right;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::end_operator_expr(const EndOperatorExpr&) const {
  return "end";
}

std::string StringVisitor::scalar_type(const ScalarType& type) const {
  auto str = std::string(string_registry->at(type.identifier));
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
  auto identifier_strs = string_registry->collect(given.identifiers);
  std::string given_str("given ");
  maybe_colorize(given_str, given.source_token.type);
  return given_str + "<" + join(identifier_strs, ", ") + "> " + given.declaration->accept(*this);
}

std::string StringVisitor::type_let(const TypeLet& let) const {
  auto identifier_str = std::string(string_registry->at(let.identifier));
  std::string let_str("let ");
  maybe_colorize(let_str, let.source_token.type);
  return let_str + identifier_str + " = " + let.equal_to_type->accept(*this);
}

std::string StringVisitor::type_begin(const TypeBegin& begin) const {
  std::string base_str = "begin";
  if (begin.is_exported) {
    base_str += " export";
  }

  maybe_colorize(base_str, begin.source_token.type);
  base_str += "\n";
  enter_block();

  std::vector<std::string> lines;
  for (const auto& annot : begin.contents) {
    lines.emplace_back(tab_str() + annot->accept(*this));
  }

  exit_block();
  auto contents_str = join(lines, "\n");
  auto end = "\n" + tab_str() + end_str();
  return base_str + contents_str + end;
}

std::string StringVisitor::type_annot_macro(const TypeAnnotMacro& type) const {
  return tab_str() + std::string(type.source_token.lexeme) + " " + type.annotation->accept(*this);
}

}
