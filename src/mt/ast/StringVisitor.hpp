#pragma once

#include "ast.hpp"
#include "def.hpp"
#include "stmt.hpp"
#include "expr.hpp"
#include "type_annot.hpp"
#include "../string.hpp"

namespace mt {

class StringVisitor {
public:
  explicit StringVisitor(const StringRegistry* string_registry) :
    parenthesize_exprs(true),
    include_identifier_classification(true),
    include_def_ptrs(true),
    colorize(true),
    tab_depth(-1), string_registry(string_registry) {
    //
  }
  ~StringVisitor() = default;

  std::string class_def(const ClassDef& def) const;
  std::string variable_def(const VariableDef& def) const;
  std::string function_def(const FunctionDef& def) const;
  std::string function_header(const FunctionHeader& header) const;
  std::string block(const Block& block) const;
  std::string root_block(const RootBlock& block) const;
  std::string function_reference(const FunctionReference& reference) const;

  std::string properties(const ClassDef::Properties& properties) const;
  std::string property(const ClassDef::Property& prop) const;
  std::string methods(const ClassDef::Methods& meths) const;

  std::string expr_stmt(const ExprStmt& stmt) const;
  std::string assignment_stmt(const AssignmentStmt& stmt) const;
  std::string for_stmt(const ForStmt& stmt) const;
  std::string if_stmt(const IfStmt& stmt) const;
  std::string control_stmt(const ControlStmt& stmt) const;
  std::string while_stmt(const WhileStmt& stmt) const;
  std::string switch_stmt(const SwitchStmt& stmt) const;
  std::string try_stmt(const TryStmt& stmt) const;
  std::string command_stmt(const CommandStmt& stmt) const;
  std::string variable_declaration_stmt(const VariableDeclarationStmt& stmt) const;

  std::string if_branch(const IfBranch& branch, const char* branch_prefix) const;
  std::string else_branch(const ElseBranch& branch) const;

  std::string variable_reference_expr(const VariableReferenceExpr& expr) const;
  std::string function_call_expr(const FunctionCallExpr& expr) const;
  std::string function_reference_expr(const FunctionReferenceExpr& expr) const;
  std::string anonymous_function_expr(const AnonymousFunctionExpr& expr) const;
  std::string colon_subscript_expr(const ColonSubscriptExpr& expr) const;
  std::string char_literal_expr(const CharLiteralExpr& expr) const;
  std::string string_literal_expr(const StringLiteralExpr& expr) const;
  std::string number_literal_expr(const NumberLiteralExpr& expr) const;
  std::string ignore_function_argument_expr(const IgnoreFunctionArgumentExpr& expr) const;
  std::string dynamic_field_reference_expr(const DynamicFieldReferenceExpr& expr) const;
  std::string literal_field_reference_expr(const LiteralFieldReferenceExpr& expr) const;
  std::string subscript_expr(const Subscript& expr) const;
  std::string identifier_reference_expr(const IdentifierReferenceExpr& expr) const;
  std::string grouping_expr_component(const GroupingExprComponent& expr, bool include_delimiter) const;
  std::string grouping_expr(const GroupingExpr& expr) const;
  std::string unary_operator_expr(const UnaryOperatorExpr& expr) const;
  std::string binary_operator_expr(const BinaryOperatorExpr& expr) const;
  std::string end_operator_expr(const EndOperatorExpr& expr) const;

  std::string union_type(const UnionType& type) const;
  std::string scalar_type(const ScalarType& type) const;
  std::string function_type(const FunctionType& type) const;
  std::string type_begin(const TypeBegin& begin) const;
  std::string type_given(const TypeGiven& given) const;
  std::string type_let(const TypeLet& let) const;
  std::string inline_type(const InlineType& type) const;
  std::string type_annot_macro(const TypeAnnotMacro& type) const;

private:
  void maybe_parenthesize(std::string& str) const;
  void maybe_colorize(std::string& str, TokenType type) const;
  void maybe_colorize(std::string& str, int color_code_index) const;
  void maybe_colorize(std::string& str, const char* color_code) const;

  std::string tab_str() const;
  std::string end_str() const;

  std::string subscripts(const std::vector<Subscript>& subs) const;

  template <typename T>
  std::string visit_array(const std::vector<T>& visitables, const std::string& delim) const;

  void enter_block() const;
  void exit_block() const;

public:
  bool parenthesize_exprs;
  bool include_identifier_classification;
  bool include_def_ptrs;
  bool colorize;

private:
  mutable int tab_depth;
  const StringRegistry* string_registry;
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