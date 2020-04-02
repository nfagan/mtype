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

}
