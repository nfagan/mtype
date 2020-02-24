#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"
#include "visitor.hpp"

namespace mt {

/*
 * ClassDefNode
 */

void ClassDefNode::accept(TypePreservingVisitor& vis) {
  vis.class_def_node(*this);
}

void ClassDefNode::accept_const(TypePreservingVisitor& vis) const {
  vis.class_def_node(*this);
}

ClassDefNode* ClassDefNode::accept(IdentifierClassifier& classifier) {
  return classifier.class_def_node(*this);
}

std::string ClassDefNode::accept(const StringVisitor& vis) const {
  return vis.class_def_node(*this);
}

/*
 * FunctionDefNode
 */

void FunctionDefNode::accept(TypePreservingVisitor& vis) {
  vis.function_def_node(*this);
}

void FunctionDefNode::accept_const(TypePreservingVisitor& vis) const {
  vis.function_def_node(*this);
}

std::string FunctionDefNode::accept(const StringVisitor& vis) const {
  return vis.function_def_node(*this);
}

FunctionDefNode* FunctionDefNode::accept(IdentifierClassifier& classifier) {
  return classifier.function_def_node(*this);
}

}