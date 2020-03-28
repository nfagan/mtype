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

std::string TypeGiven::accept(const StringVisitor& vis) const {
  return vis.type_given(*this);
}

std::string TypeLet::accept(const StringVisitor& vis) const {
  return vis.type_let(*this);
}

std::string TypeBegin::accept(const StringVisitor& vis) const {
  return vis.type_begin(*this);
}

std::string UnionType::accept(const StringVisitor& vis) const {
  return vis.union_type(*this);
}

std::string ScalarType::accept(const StringVisitor& vis) const {
  return vis.scalar_type(*this);
}

std::string FunctionType::accept(const StringVisitor& vis) const {
  return vis.function_type(*this);
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
