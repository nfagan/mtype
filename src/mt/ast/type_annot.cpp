#include "type_annot.hpp"
#include "StringVisitor.hpp"
#include "visitor.hpp"
#include "../parse/identifier_classification.hpp"
#include <cassert>

namespace mt {

/*
 * TypeAnnotMacro
 */

std::string TypeAnnotMacro::accept(const StringVisitor& vis) const {
  return vis.type_annot_macro(*this);
}

TypeAnnotMacro* TypeAnnotMacro::accept(IdentifierClassifier& classifier) {
  return classifier.type_annot_macro(*this);
}

void TypeAnnotMacro::accept_const(TypePreservingVisitor& vis) const {
  return vis.type_annot_macro(*this);
}

void TypeAnnotMacro::accept(TypePreservingVisitor& vis) {
  return vis.type_annot_macro(*this);
}

/*
 * ConstructorTypeNode
 */

std::string ConstructorTypeNode::accept(const StringVisitor& vis) const {
  return vis.constructor_type_node(*this);
}

ConstructorTypeNode* ConstructorTypeNode::accept(IdentifierClassifier& classifier) {
  return classifier.constructor_type_node(*this);
}

void ConstructorTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.constructor_type_node(*this);
}

void ConstructorTypeNode::accept(TypePreservingVisitor& vis) {
  vis.constructor_type_node(*this);
}

/*
 * NamespaceTypeNode
 */

std::string NamespaceTypeNode::accept(const StringVisitor& vis) const {
  return vis.namespace_type_node(*this);
}

void NamespaceTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.namespace_type_node(*this);
}

void NamespaceTypeNode::accept(TypePreservingVisitor& vis) {
  vis.namespace_type_node(*this);
}

/*
 * DeclareTypeNode
 */

DeclareTypeNode::Method::Method() : kind(Kind::function) {
  //
}

DeclareTypeNode::Method::Method(const TypeIdentifier& name, BoxedType type) :
kind(Kind::function), name(name), type(std::move(type)) {
  //
}

DeclareTypeNode::Method::Method(UnaryOperator op, BoxedType type) :
kind(Kind::unary_operator), unary_operator(op), type(std::move(type)) {
  //
}

DeclareTypeNode::Method::Method(BinaryOperator op, BoxedType type) :
kind(Kind::binary_operator), binary_operator(op), type(std::move(type)) {
  //
}

std::string DeclareTypeNode::Method::function_name(const StringRegistry& str_registry) const {
  switch (kind) {
    case Kind::binary_operator:
      return to_symbol(binary_operator);
    case Kind::unary_operator:
      return to_symbol(unary_operator);
    default:
      return str_registry.at(name.full_name());
  }
}

std::string DeclareTypeNode::accept(const StringVisitor& vis) const {
  return vis.declare_type_node(*this);
}

void DeclareTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.declare_type_node(*this);
}

void DeclareTypeNode::accept(TypePreservingVisitor& vis) {
  vis.declare_type_node(*this);
}

Optional<DeclareTypeNode::Kind> DeclareTypeNode::kind_from_string(std::string_view str) {
  if (str == "scalar") {
    return Optional<Kind>(Kind::scalar);
  } else if (str == "method") {
    return Optional<Kind>(Kind::method);
  } else {
    return NullOpt{};
  }
}

const char* DeclareTypeNode::kind_to_string(Kind kind) {
  switch (kind) {
    case Kind::scalar:
      return "scalar";
    case Kind::method:
      return "method";
  }

  assert(false);
  return "";
}

/*
 * DeclareFunctionTypeNode
 */

std::string DeclareFunctionTypeNode::accept(const StringVisitor& vis) const {
  return vis.declare_function_type_node(*this);
}

void DeclareFunctionTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.declare_function_type_node(*this);
}

void DeclareFunctionTypeNode::accept(TypePreservingVisitor& vis) {
  vis.declare_function_type_node(*this);
}

/*
 * DeclareClassTypeNode
 */

std::string DeclareClassTypeNode::accept(const StringVisitor& vis) const {
  return vis.declare_class_type_node(*this);
}

void DeclareClassTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.declare_class_type_node(*this);
}

void DeclareClassTypeNode::accept(TypePreservingVisitor& vis) {
  vis.declare_class_type_node(*this);
}

/*
 * TypeImportNode
 */

std::string TypeImportNode::accept(const StringVisitor& vis) const {
  return vis.type_import_node(*this);
}

/*
 * TypeAssertion
 */

std::string TypeAssertion::accept(const StringVisitor& vis) const {
  return vis.type_assertion(*this);
}

TypeAssertion* TypeAssertion::accept(IdentifierClassifier& classifier) {
  return classifier.type_assertion(*this);
}

void TypeAssertion::accept_const(TypePreservingVisitor& vis) const {
  vis.type_assertion(*this);
}

void TypeAssertion::accept(TypePreservingVisitor& vis) {
  vis.type_assertion(*this);
}

Optional<AstNode*> TypeAssertion::enclosed_code_ast_node() const {
  return Optional<AstNode*>(node.get());
}

Optional<TypeAssertion*> TypeAssertion::extract_type_assertion() {
  return Optional<TypeAssertion*>(this);
}

/*
 * TypeGiven
 */

std::string SchemeTypeNode::accept(const StringVisitor& vis) const {
  return vis.scheme_type_node(*this);
}

void SchemeTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.scheme_type_node(*this);
}

void SchemeTypeNode::accept(TypePreservingVisitor& vis) {
  vis.scheme_type_node(*this);
}

/*
 * TypeLet
 */

std::string TypeLet::accept(const StringVisitor& vis) const {
  return vis.type_let(*this);
}

void TypeLet::accept_const(TypePreservingVisitor& vis) const {
  vis.type_let(*this);
}

void TypeLet::accept(TypePreservingVisitor& vis) {
  vis.type_let(*this);
}

/*
 * TypeBegin
 */

std::string TypeBegin::accept(const StringVisitor& vis) const {
  return vis.type_begin(*this);
}

void TypeBegin::accept_const(TypePreservingVisitor& vis) const {
  vis.type_begin(*this);
}

void TypeBegin::accept(TypePreservingVisitor& vis) {
  vis.type_begin(*this);
}

/*
 * RecordTypeNode
 */

std::string RecordTypeNode::accept(const StringVisitor& vis) const {
  return vis.record_type_node(*this);
}

void RecordTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.record_type_node(*this);
}

void RecordTypeNode::accept(TypePreservingVisitor& vis) {
  vis.record_type_node(*this);
}

/*
 * InferTypeNode
 */

std::string InferTypeNode::accept(const StringVisitor& vis) const {
  return vis.infer_type_node(*this);
}

void InferTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.infer_type_node(*this);
}

void InferTypeNode::accept(TypePreservingVisitor& vis) {
  vis.infer_type_node(*this);
}

/*
 * UnionTypeNode
 */

std::string UnionTypeNode::accept(const StringVisitor& vis) const {
  return vis.union_type_node(*this);
}

void UnionTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.union_type_node(*this);
}

void UnionTypeNode::accept(TypePreservingVisitor& vis) {
  vis.union_type_node(*this);
}

/*
 * TupleTypeNode
 */

std::string TupleTypeNode::accept(const StringVisitor& vis) const {
  return vis.tuple_type_node(*this);
}

void TupleTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.tuple_type_node(*this);
}

void TupleTypeNode::accept(TypePreservingVisitor& vis) {
  vis.tuple_type_node(*this);
}

/*
 * ListTypeNode
 */

std::string ListTypeNode::accept(const StringVisitor& vis) const {
  return vis.list_type_node(*this);
}

void ListTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.list_type_node(*this);
}

void ListTypeNode::accept(TypePreservingVisitor& vis) {
  vis.list_type_node(*this);
}

/*
 * ScalarTypeNode
 */

std::string ScalarTypeNode::accept(const StringVisitor& vis) const {
  return vis.scalar_type_node(*this);
}

void ScalarTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.scalar_type_node(*this);
}
void ScalarTypeNode::accept(TypePreservingVisitor& vis) {
  vis.scalar_type_node(*this);
}

/*
 * FunctionTypeNode
 */

std::string FunctionTypeNode::accept(const StringVisitor& vis) const {
  return vis.function_type_node(*this);
}

void FunctionTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.function_type_node(*this);
}

void FunctionTypeNode::accept(TypePreservingVisitor& vis) {
  vis.function_type_node(*this);
}

/*
 * FunTypeNode
 */

std::string FunTypeNode::accept(const StringVisitor& vis) const {
  return vis.fun_type_node(*this);
}

void FunTypeNode::accept(TypePreservingVisitor& vis) {
  vis.fun_type_node(*this);
}

void FunTypeNode::accept_const(TypePreservingVisitor& vis) const {
  vis.fun_type_node(*this);
}

FunTypeNode* FunTypeNode::accept(IdentifierClassifier& classifier) {
  return classifier.fun_type_node(*this);
}

Optional<AstNode*> FunTypeNode::enclosed_code_ast_node() const {
  return Optional<AstNode*>(definition.get());
}

}
