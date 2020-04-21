#pragma once

#include "ast.hpp"
#include "def.hpp"
#include "stmt.hpp"
#include "expr.hpp"
#include "type_annot.hpp"
#include "../store.hpp"
#include "../string.hpp"

namespace mt {

class StringVisitor {
public:
  explicit StringVisitor(const StringRegistry* string_registry,
                         const Store* store) :
    parenthesize_exprs(true),
    include_identifier_classification(true),
    include_def_ptrs(true),
    colorize(true),
    tab_depth(-1),
    string_registry(string_registry),
    store_reader(*store) {
    //
  }
  ~StringVisitor() = default;

  std::string class_def_node(const ClassDefNode& ref) const;
  std::string block(const Block& block) const;
  std::string root_block(const RootBlock& block) const;
  std::string function_def_node(const FunctionDefNode& reference) const;
  std::string property_node(const PropertyNode& node) const;
  std::string method_node(const MethodNode& node) const;

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

  std::string superclass_method_reference_expr(const SuperclassMethodReferenceExpr& expr) const;
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
  std::string grouping_expr(const GroupingExpr& expr) const;
  std::string unary_operator_expr(const UnaryOperatorExpr& expr) const;
  std::string binary_operator_expr(const BinaryOperatorExpr& expr) const;
  std::string end_operator_expr(const EndOperatorExpr& expr) const;

  std::string namespace_type_node(const NamespaceTypeNode& type) const;
  std::string record_type_node(const RecordTypeNode& type) const;
  std::string union_type_node(const UnionTypeNode& type) const;
  std::string list_type_node(const ListTypeNode& type) const;
  std::string tuple_type_node(const TupleTypeNode& type) const;
  std::string scalar_type_node(const ScalarTypeNode& type) const;
  std::string function_type_node(const FunctionTypeNode& type) const;
  std::string type_begin(const TypeBegin& begin) const;
  std::string type_given(const TypeGiven& given) const;
  std::string type_let(const TypeLet& let) const;
  std::string inline_type(const InlineType& type) const;
  std::string type_annot_macro(const TypeAnnotMacro& type) const;
  std::string fun_type_node(const FunTypeNode& node) const;
  std::string type_assertion(const TypeAssertion& assertion) const;
  std::string type_import_node(const TypeImportNode& import) const;
  std::string declare_type_node(const DeclareTypeNode& node) const;
  std::string constructor_type_node(const ConstructorTypeNode& node) const;
  std::string infer_type_node(const InferTypeNode& node) const;

private:
  std::string variable_def(const VariableDef& def) const;
  std::string function_def(const FunctionDef& def) const;
  std::string function_header(const FunctionHeader& header) const;

  std::string properties(const BoxedPropertyNodes& properties) const;
  std::string methods(const BoxedMethodNodes& nodes) const;

  std::string if_branch(const IfBranch& branch, const char* branch_prefix) const;
  std::string else_branch(const ElseBranch& branch) const;

  std::string grouping_expr_component(const GroupingExprComponent& expr, bool include_delimiter) const;

  std::string type_annotation_block(const BoxedTypeAnnots& annots) const;

  void maybe_parenthesize(std::string& str) const;
  void maybe_colorize(std::string& str, TokenType type) const;
  void maybe_colorize(std::string& str, int color_code_index) const;
  void maybe_colorize(std::string& str, const char* color_code) const;

  std::string tab_str() const;
  std::string end_str() const;

  std::string subscripts(const std::vector<Subscript>& subs) const;
  std::string function_input_parameters(const std::vector<FunctionInputParameter>& inputs) const;

  template <typename T>
  std::string visit_array(const T& visitables, const std::string& delim) const;

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
  Store::ReadConst store_reader;
};

}

namespace mt {

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
std::string mt::StringVisitor::visit_array(const T& visitables,
                                           const std::string& delim) const {
  std::vector<std::string> values;
  for (const auto& arg : visitables) {
    values.emplace_back(detail::accept_impl(*this, arg));
  }
  return mt::join(values, delim);
}

}