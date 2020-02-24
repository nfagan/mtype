#pragma once

#define MT_VISITOR_METHOD(name, type) \
  virtual void name(type&) {} \
  virtual void name(const type&) {}

namespace mt {

struct RootBlock;
struct Block;

struct PresumedSuperclassMethodReferenceExpr;
struct AnonymousFunctionExpr;
struct FunctionReferenceExpr;
struct ColonSubscriptExpr;
struct StringLiteralExpr;
struct CharLiteralExpr;
struct NumberLiteralExpr;
struct IgnoreFunctionArgumentExpr;
struct DynamicFieldReferenceExpr;
struct LiteralFieldReferenceExpr;
struct FunctionCallExpr;
struct VariableReferenceExpr;
struct IdentifierReferenceExpr;
struct GroupingExpr;
struct EndOperatorExpr;
struct UnaryOperatorExpr;
struct BinaryOperatorExpr;

struct VariableDeclarationStmt;
struct CommandStmt;
struct TryStmt;
struct SwitchStmt;
struct WhileStmt;
struct ControlStmt;
struct ForStmt;
struct IfStmt;
struct AssignmentStmt;
struct ExprStmt;

struct ClassDefNode;
struct FunctionDefNode;

class TypePreservingVisitor {
public:
  MT_VISITOR_METHOD(root_block, RootBlock)
  MT_VISITOR_METHOD(block, Block)

  MT_VISITOR_METHOD(presumed_superclass_method_reference_expr, PresumedSuperclassMethodReferenceExpr)
  MT_VISITOR_METHOD(anonymous_function_expr, AnonymousFunctionExpr)
  MT_VISITOR_METHOD(function_reference_expr, FunctionReferenceExpr)
  MT_VISITOR_METHOD(colon_subscript_expr, ColonSubscriptExpr)
  MT_VISITOR_METHOD(char_literal_expr, CharLiteralExpr)
  MT_VISITOR_METHOD(string_literal_expr, StringLiteralExpr)
  MT_VISITOR_METHOD(number_literal_expr, NumberLiteralExpr)
  MT_VISITOR_METHOD(ignore_function_argument_expr, IgnoreFunctionArgumentExpr)
  MT_VISITOR_METHOD(dynamic_field_reference_expr, DynamicFieldReferenceExpr)
  MT_VISITOR_METHOD(literal_field_reference_expr, LiteralFieldReferenceExpr)
  MT_VISITOR_METHOD(function_call_expr, FunctionCallExpr)
  MT_VISITOR_METHOD(variable_reference_expr, VariableReferenceExpr)
  MT_VISITOR_METHOD(identifier_reference_expr, IdentifierReferenceExpr)
  MT_VISITOR_METHOD(grouping_expr, GroupingExpr)
  MT_VISITOR_METHOD(end_operator_expr, EndOperatorExpr)
  MT_VISITOR_METHOD(unary_operator_expr, UnaryOperatorExpr)
  MT_VISITOR_METHOD(binary_operator_expr, BinaryOperatorExpr)

  MT_VISITOR_METHOD(variable_declaration_stmt, VariableDeclarationStmt)
  MT_VISITOR_METHOD(command_stmt, CommandStmt)
  MT_VISITOR_METHOD(try_stmt, TryStmt)
  MT_VISITOR_METHOD(switch_stmt, SwitchStmt)
  MT_VISITOR_METHOD(while_stmt, WhileStmt)
  MT_VISITOR_METHOD(control_stmt, ControlStmt)
  MT_VISITOR_METHOD(for_stmt, ForStmt)
  MT_VISITOR_METHOD(if_stmt, IfStmt)
  MT_VISITOR_METHOD(assignment_stmt, AssignmentStmt)
  MT_VISITOR_METHOD(expr_stmt, ExprStmt)

  MT_VISITOR_METHOD(class_def_node, ClassDefNode)
  MT_VISITOR_METHOD(function_def_node, FunctionDefNode)
};

#undef MT_VISITOR_METHOD

}