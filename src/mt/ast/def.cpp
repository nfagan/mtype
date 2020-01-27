#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"
#include "../ast_gen.hpp"

namespace mt {

RootBlock* RootBlock::accept(IdentifierClassifier& classifier) {
  return classifier.root_block(*this);
}

std::string RootBlock::accept(const StringVisitor& vis) const {
  return vis.root_block(*this);
}

std::string Block::accept(const StringVisitor& vis) const {
  return vis.block(*this);
}

Block* Block::accept(IdentifierClassifier& classifier) {
  return classifier.block(*this);
}

std::string FunctionDef::accept(const StringVisitor& vis) const {
  return vis.function_def(*this);
}

Def* FunctionDef::accept(IdentifierClassifier& classifier) {
  return classifier.function_def(*this);
}

std::string VariableDef::accept(const mt::StringVisitor& vis) const {
  return vis.variable_def(*this);
}

}