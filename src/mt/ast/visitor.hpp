#pragma once

#define MT_VISITOR_METHOD(name, type) \
  virtual void name(type&) {} \
  virtual void name(const type&) {}

namespace mt {

struct RootBlock;
struct Block;

struct SuperclassMethodReferenceExpr;
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
struct PropertyNode;
struct MethodNode;

struct RecordTypeNode;
struct UnionTypeNode;
struct TupleTypeNode;
struct ListTypeNode;
struct ScalarTypeNode;
struct FunctionTypeNode;
struct FunTypeNode;
struct TypeBegin;
struct SchemeTypeNode;
struct TypeLet;
struct TypeAnnotMacro;
struct TypeAssertion;
struct DeclareTypeNode;
struct DeclareFunctionTypeNode;
struct DeclareClassTypeNode;
struct NamespaceTypeNode;
struct ConstructorTypeNode;
struct InferTypeNode;
struct CastTypeNode;

class TypePreservingVisitor {
public:
  MT_VISITOR_METHOD(root_block, RootBlock)
  MT_VISITOR_METHOD(block, Block)

  MT_VISITOR_METHOD(superclass_method_reference_expr, SuperclassMethodReferenceExpr)
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
  MT_VISITOR_METHOD(property_node, PropertyNode)
  MT_VISITOR_METHOD(method_node, MethodNode)

  MT_VISITOR_METHOD(cast_type_node, CastTypeNode)
  MT_VISITOR_METHOD(infer_type_node, InferTypeNode)
  MT_VISITOR_METHOD(constructor_type_node, ConstructorTypeNode)
  MT_VISITOR_METHOD(namespace_type_node, NamespaceTypeNode)
  MT_VISITOR_METHOD(declare_type_node, DeclareTypeNode)
  MT_VISITOR_METHOD(declare_function_type_node, DeclareFunctionTypeNode)
  MT_VISITOR_METHOD(declare_class_type_node, DeclareClassTypeNode)
  MT_VISITOR_METHOD(record_type_node, RecordTypeNode)
  MT_VISITOR_METHOD(union_type_node, UnionTypeNode)
  MT_VISITOR_METHOD(tuple_type_node, TupleTypeNode)
  MT_VISITOR_METHOD(list_type_node, ListTypeNode)
  MT_VISITOR_METHOD(scalar_type_node, ScalarTypeNode)
  MT_VISITOR_METHOD(function_type_node, FunctionTypeNode)
  MT_VISITOR_METHOD(fun_type_node, FunTypeNode)
  MT_VISITOR_METHOD(scheme_type_node, SchemeTypeNode)
  MT_VISITOR_METHOD(type_begin, TypeBegin)
  MT_VISITOR_METHOD(type_let, TypeLet)
  MT_VISITOR_METHOD(type_annot_macro, TypeAnnotMacro)
  MT_VISITOR_METHOD(type_assertion, TypeAssertion)
};

#undef MT_VISITOR_METHOD

}