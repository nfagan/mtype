#include "ast.hpp"
#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"

namespace mt {

void MatlabScope::register_import(Import&& import) {
  imports.emplace_back(std::move(import));
}

bool MatlabScope::register_local_function(int64_t name, FunctionReference* ref) {
  if (local_functions.count(name) > 0) {
    return false;
  } else {
    local_functions[name] = ref;
    return true;
  }
}

void MatlabScope::register_local_variable(int64_t name, std::unique_ptr<VariableDef> def) {
  local_variables[name] = std::move(def);
}


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

std::string FunctionReference::accept(const StringVisitor& vis) const {
  return vis.function_reference(*this);
}

FunctionReference* FunctionReference::accept(IdentifierClassifier& classifier) {
  return classifier.function_reference(*this);
}

}