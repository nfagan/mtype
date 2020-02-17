#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"

namespace mt {

ClassDefNode* ClassDefNode::accept(IdentifierClassifier& classifier) {
  return classifier.class_def_node(*this);
}

std::string ClassDefNode::accept(const StringVisitor& vis) const {
  return vis.class_def_node(*this);
}

std::string ClassDef::accept(const StringVisitor& vis) const {
  return vis.class_def(*this);
}

std::string FunctionDef::accept(const StringVisitor& vis) const {
  return vis.function_def(*this);
}

std::string VariableDef::accept(const mt::StringVisitor& vis) const {
  return vis.variable_def(*this);
}

std::string FunctionDefNode::accept(const StringVisitor& vis) const {
  return vis.function_def_node(*this);
}

FunctionDefNode* FunctionDefNode::accept(IdentifierClassifier& classifier) {
  return classifier.function_def_node(*this);
}

}