#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"
#include "../ast_gen.hpp"

namespace mt {

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