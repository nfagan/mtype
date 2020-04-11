#include "type_annot.hpp"
#include "StringVisitor.hpp"
#include "visitor.hpp"
#include "../identifier_classification.hpp"

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
 * DeclareTypeNode
 */

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
  } else {
    return NullOpt{};
  }
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

/*
 * InlineType
 */

std::string InlineType::accept(const StringVisitor& vis) const {
  return vis.inline_type(*this);
}

void InlineType::accept_const(TypePreservingVisitor& vis) const {
  vis.inline_type(*this);
}

void InlineType::accept(TypePreservingVisitor& vis) {
  vis.inline_type(*this);
}

/*
 * TypeGiven
 */

std::string TypeGiven::accept(const StringVisitor& vis) const {
  return vis.type_given(*this);
}

void TypeGiven::accept_const(TypePreservingVisitor& vis) const {
  vis.type_given(*this);
}

void TypeGiven::accept(TypePreservingVisitor& vis) {
  vis.type_given(*this);
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
