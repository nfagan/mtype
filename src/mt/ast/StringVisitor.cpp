#include "StringVisitor.hpp"
#include "../display.hpp"
#include <algorithm>
#include <cassert>

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

void StringVisitor::maybe_colorize(std::string& str, TokenType type) const {
  using namespace style;
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
  using namespace style;

  const auto all = all_colors();
  const char* color_code = dflt;
  if (color_code_index >= 0 && color_code_index < int(all.size())) {
    color_code = all[color_code_index];
  }
  maybe_colorize(str, color_code);
}

void StringVisitor::maybe_colorize(std::string& str, const char* color_code) const {
  if (colorize) {
    str = color_code + str + style::dflt;
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

std::string StringVisitor::function_parameters(const FunctionParameters& params,
                                               const char* open,
                                               const char* close) const {
  std::string str(open);
  for (int64_t i = 0; i < int64_t(params.size()); i++) {
    const auto& param = params[i];
    if (param.is_ignored()) {
      str += "~";
    } else {
      str += string_registry->at(param.name.full_name());
    }
    if (i < int64_t(params.size()) - 1) {
      str += ", ";
    }
  }
  str += close;
  return str;
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
  auto outputs = function_parameters(header.outputs, "[", "]");
  auto inputs = function_parameters(header.inputs, "(", ")");
  auto name = string_registry->at(header.name.full_name());
  auto func = std::string("function");
  maybe_colorize(func, TokenType::keyword_function);
  return func + outputs + " = " + name + inputs;
}

std::string StringVisitor::function_def_node(const FunctionDefNode& def_node) const {
  const auto& def_handle = def_node.def_handle;

  std::string ptr_str;
  if (include_def_ptrs) {
    ptr_str = "<" + std::to_string(def_handle.get_index()) + "> ";
  }

  std::string def_str;
  if (def_handle.is_valid()) {
    const auto& def = store_reader.at(def_handle);
    def_str = function_def(def);
  }

  return tab_str() + ptr_str + def_str;
}

std::string StringVisitor::function_def(const FunctionDef& def) const {
  const auto header = function_header(def.header);
  std::string body;
  if (def.body) {
    body = def.body->accept(*this);
  }
  auto end = tab_str() + end_str();
  return header + "\n" + body + "\n" + end;
}

std::string StringVisitor::class_def_node(const ClassDefNode& ref) const {
  const auto& def = store_reader.at(ref.handle);

  std::string class_def_kw = tab_str() + "classdef";
  maybe_colorize(class_def_kw, TokenType::keyword_classdef);
  auto result = class_def_kw + " " + std::string(string_registry->at(def.name.full_name()));

  if (!def.superclasses.empty()) {
    result += " < ";
    for (int64_t i = 0; i < int64_t(def.superclasses.size()); i++) {
      result += string_registry->at(def.superclasses[i].name.full_name());
      if (i < int64_t(def.superclasses.size()) - 1) {
        result += " & ";
      }
    }
  }

  result += "\n";
  result += properties(ref.properties) + "\n";
  result += methods(ref.method_defs) + "\n";
  result += tab_str() + end_str();

  return result;
}

std::string StringVisitor::property_node(const PropertyNode& node) const {
  auto prop_str = string_registry->at(node.name.full_name());

  if (node.type) {
    prop_str += " :: " + node.type->accept(*this);
  }

  if (node.initializer) {
    prop_str += " = ";
    prop_str += node.initializer->accept(*this);
  }

  prop_str += ";";
  return prop_str;
}

std::string StringVisitor::method_node(const MethodNode& node) const {
  std::string str = ([&]() {
    if (node.type) {
      return tab_str() + node.type->accept(*this) + "\n" + node.def->accept(*this);
    } else {
      return node.def->accept(*this);
    }
  })();

  if (node.external_block) {
    str += "\n" + node.external_block->accept(*this);
  }

  return str;
}

std::string StringVisitor::properties(const BoxedPropertyNodes& properties) const {
  enter_block();

  std::string prop_str = "properties\n";
  maybe_colorize(prop_str, TokenType::keyword_properties);
  prop_str = tab_str() + prop_str;

  enter_block();

  for (const auto& prop : properties) {
    prop_str += tab_str() + prop->accept(*this) + "\n";
  }

  exit_block();
  prop_str += tab_str() + end_str();
  exit_block();

  return prop_str;
}

std::string StringVisitor::methods(const BoxedMethodNodes& nodes) const {
  enter_block();

  std::string method_str("methods");
  method_str = tab_str() + method_str + "\n";
  maybe_colorize(method_str, TokenType::keyword_methods);

  enter_block();

  std::string func_def_strs;
  for (const auto& node : nodes) {
    func_def_strs += node->accept(*this) + "\n";
  }

  exit_block();
  method_str += func_def_strs + tab_str() + end_str();
  exit_block();

  return method_str;
}

std::string StringVisitor::variable_def(const VariableDef& def) const {
  return tab_str() + string_registry->at(def.name.full_name());
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
  std::string loop_var_str = string_registry->at(stmt.loop_variable_identifier.full_name());
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
  auto str = tab_str() + string_registry->at(stmt.command_identifier);
  for (const auto& arg : stmt.arguments) {
    str += (" " + std::string(arg.source_token.lexeme));
  }
  str += ";";
  return str;
}

std::string StringVisitor::variable_declaration_stmt(const VariableDeclarationStmt& stmt) const {
  //  @TODO: Derive declaration qualifier.
  std::vector<std::string> identifier_strs;
  for (const auto& id : stmt.identifiers) {
    identifier_strs.emplace_back(string_registry->at(id.full_name()));
  }
  std::string decl_keyword(stmt.source_token.lexeme);
  maybe_colorize(decl_keyword, stmt.source_token.type);

  return tab_str() + decl_keyword + " " + join(identifier_strs, " ");
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
  std::string ptr_str;

  if (include_def_ptrs) {
    ptr_str += "<" + std::to_string(expr.handle.get_index()) + ":";

    if (expr.handle.is_valid()) {
      const auto& def_handle = store_reader.at(expr.handle).def_handle;
      ptr_str += std::to_string(def_handle.get_index());
    } else {
      ptr_str += "-1";
    }

    ptr_str += ">";
  }

  auto str = "@" + ptr_str + string_registry->at(expr.identifier.full_name());
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::variable_reference_expr(const VariableReferenceExpr& expr) const {
  std::string str = std::string(string_registry->at(expr.name.full_name()));
  std::string sub_str = subscripts(expr.subscripts);

  if (expr.is_initializer) {
    maybe_colorize(str, 3);
  }

  if (include_def_ptrs) {
    str = "<" + std::to_string(expr.def_handle.get_index()) + " " + str + ">";
  }

  str += sub_str;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::function_call_expr(const FunctionCallExpr& expr) const {
  const auto& ref = store_reader.at(expr.reference_handle);
  auto name = string_registry->at(ref.name.full_name());
  const auto arg_str = visit_array(expr.arguments, ", ");
  auto sub_str = subscripts(expr.subscripts);

  const std::string call_expr_str = "(" + arg_str + ")" + sub_str;

  if (include_identifier_classification) {
    const auto& def_handle = ref.def_handle;
    int color_code = def_handle.is_valid() ? 1 : 0;
    maybe_colorize(name, color_code);

    if (include_def_ptrs) {
      auto ptr_str = std::to_string(expr.reference_handle.get_index());
      return "<" + ptr_str + " " + name + ">" + call_expr_str;
    } else {
      return name + call_expr_str;
    }
  } else {
    return name + call_expr_str;
  }
}

std::string StringVisitor::anonymous_function_expr(const AnonymousFunctionExpr& expr) const {
  const auto inputs = function_parameters(expr.inputs, "(", ")");
  auto header = std::string("@") + inputs + " ";
  auto body = expr.expr->accept(*this);
  auto str = header + body;
  maybe_parenthesize(str);
  return str;
}

std::string StringVisitor::superclass_method_reference_expr(const SuperclassMethodReferenceExpr& expr) const {
  auto identifier_str = std::string(string_registry->at(expr.invoking_argument_name.full_name()));
  return identifier_str + "@" + expr.superclass_reference_expr->accept(*this);
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
  return string_registry->at(expr.field_identifier);
}

std::string StringVisitor::identifier_reference_expr(const IdentifierReferenceExpr& expr) const {
  std::string sub_str = subscripts(expr.subscripts);
  std::string str = string_registry->at(expr.primary_identifier.full_name()) + sub_str;
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

std::string StringVisitor::namespace_type_node(const NamespaceTypeNode& type) const {
  std::string ns_str("namespace ");
  maybe_colorize(ns_str, type.source_token.type);
  ns_str += string_registry->at(type.identifier.full_name()) + "\n";
  enter_block();
  ns_str += type_annotation_block(type.enclosing);
  exit_block();
  ns_str += "\n" + tab_str() + end_str();
  return ns_str;
}

std::string StringVisitor::record_type_node(const RecordTypeNode& type) const {
  std::string res("record ");
  maybe_colorize(res, type.source_token.type);
  res += string_registry->at(type.identifier.full_name());
  res += "\n";
  enter_block();

  for (const auto& field : type.fields) {
    res += tab_str() + string_registry->at(field.name.full_name());
    res += ": ";
    res += field.type->accept(*this);
    res += "\n";
  }

  exit_block();
  res += tab_str() + end_str();
  return res;
}

std::string StringVisitor::union_type_node(const UnionTypeNode& type) const {
  return visit_array(type.members, " | ");
}

std::string StringVisitor::list_type_node(const ListTypeNode& type) const {
  return "list<" + visit_array(type.pattern, ", ") + ">";
}

std::string StringVisitor::tuple_type_node(const TupleTypeNode& type) const {
  return "{" +  visit_array(type.members, ", ") + "}";
}

std::string StringVisitor::scalar_type_node(const ScalarTypeNode& type) const {
  auto str = std::string(string_registry->at(type.identifier.full_name()));
  if (!type.arguments.empty()) {
    str += ("<" + visit_array(type.arguments, ", ") + ">");
  }
  return str;
}

std::string StringVisitor::function_type_node(const FunctionTypeNode& type) const {
  auto outputs = "[" + visit_array(type.outputs, ", ") + "]";
  auto inputs = "(" + visit_array(type.inputs, ", ") + ")";
  return outputs + " = " + inputs;
}

std::string StringVisitor::fun_type_node(const FunTypeNode& node) const {
  std::string fun_str("fun");
  maybe_colorize(fun_str, node.source_token.type);
  const auto t = tab_str();
  auto tmp_tab_depth = tab_depth;
  tab_depth = 0;
  const auto full_str = tab_str() + fun_str + node.definition->accept(*this);
  tab_depth = tmp_tab_depth;
  return full_str;
}

std::string StringVisitor::type_assertion(const TypeAssertion& assertion) const {
  assert(assertion.node);
  return ":: " + assertion.has_type->accept(*this) + "\n" + assertion.node->accept(*this);
}

std::string StringVisitor::type_import_node(const TypeImportNode& import) const {
  std::string import_str("import ");
  maybe_colorize(import_str, import.source_token.type);
  return import_str + string_registry->at(import.import);
}

std::string StringVisitor::declare_type_node(const DeclareTypeNode& node) const {
  std::string declare_str("declare ");
  maybe_colorize(declare_str, node.source_token.type);
  declare_str += DeclareTypeNode::kind_to_string(node.kind);
  declare_str += " " + string_registry->at(node.identifier.full_name());

  switch (node.kind) {
    case DeclareTypeNode::Kind::scalar:
      return declare_str;
    case DeclareTypeNode::Kind::method:
      declare_str += " " + node.maybe_method.function_name(*string_registry) + " :: " +
        node.maybe_method.type->accept(*this);
      break;
  }

  return declare_str;
}

std::string StringVisitor::declare_function_type_node(const DeclareFunctionTypeNode& node) const {
  std::string declare_str("declare function ");
  maybe_colorize(declare_str, node.source_token.type);
  declare_str += string_registry->at(node.identifier.full_name());
  declare_str += " :: " + node.type->accept(*this);
  return declare_str;
}

std::string StringVisitor::declare_class_type_node(const DeclareClassTypeNode& node) const {
  std::string declare_str("declare classdef ");
  maybe_colorize(declare_str, node.source_token.type);
  declare_str += string_registry->at(node.identifier.full_name());
  declare_str += "\n" + tab_str() + node.source_type->accept(*this);
  return declare_str;
}

std::string StringVisitor::constructor_type_node(const ConstructorTypeNode& node) const {
  std::string ctor_str("constructor ");
  maybe_colorize(ctor_str, node.source_token.type);
  ctor_str += "\n" + tab_str() + node.stmt->accept(*this);
  return ctor_str;
}

std::string StringVisitor::infer_type_node(const InferTypeNode&) const {
  return "?";
}

std::string StringVisitor::cast_type_node(const CastTypeNode& node) const {
  auto cast_str = "cast " + node.to_type->accept(*this) + "\n";
  return cast_str + node.assignment_stmt->accept(*this);
}

std::string StringVisitor::scheme_type_node(const SchemeTypeNode& given) const {
  auto identifier_strs = string_registry->collect(given.identifiers);
  std::string given_str("given ");
  maybe_colorize(given_str, given.source_token.type);
  return given_str + "<" + join(identifier_strs, ", ") + "> " + given.declaration->accept(*this);
}

std::string StringVisitor::type_let(const TypeLet& let) const {
  auto identifier_str = string_registry->at(let.identifier.full_name());
  std::string let_str("let ");
  maybe_colorize(let_str, let.source_token.type);
  return let_str + identifier_str + " = " + let.equal_to_type->accept(*this);
}

std::string StringVisitor::type_annotation_block(const BoxedTypeAnnots& annots) const {
  std::vector<std::string> lines;
  for (const auto& annot : annots) {
    lines.emplace_back(tab_str() + annot->accept(*this));
  }
  return join(lines, "\n");
}

std::string StringVisitor::type_begin(const TypeBegin& begin) const {
  std::string base_str = "begin";
  if (begin.is_exported) {
    base_str += " export";
  }

  maybe_colorize(base_str, begin.source_token.type);
  base_str += "\n";
  enter_block();
  auto contents_str = type_annotation_block(begin.contents);
  exit_block();
  auto end = "\n" + tab_str() + end_str();
  return base_str + contents_str + end;
}

std::string StringVisitor::type_annot_macro(const TypeAnnotMacro& type) const {
  return tab_str() + std::string(type.source_token.lexeme) + " " + type.annotation->accept(*this);
}

}
