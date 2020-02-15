#include "ast.hpp"
#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"
#include "../store.hpp"
#include <functional>

namespace mt {

void MatlabScope::register_import(Import&& import) {
  if (import.type == ImportType::wildcard) {
    wildcard_imports.emplace_back(import);
  } else {
    fully_qualified_imports.emplace_back(import);
  }
}

bool MatlabScope::register_class(const MatlabIdentifier& name, const ClassDefHandle& handle) {
  if (classes.count(name) > 0) {
    return false;
  } else {
    classes[name] = handle;
    return true;
  }
}

bool MatlabScope::register_local_function(int64_t name, const FunctionReferenceHandle& ref) {
  if (local_functions.count(name) > 0) {
    return false;
  } else {
    local_functions[name] = ref;
    return true;
  }
}

FunctionReferenceHandle MatlabScope::lookup_local_function(const ScopeStore* store, int64_t name) const {
  auto it = local_functions.find(name);
  if (it == local_functions.end()) {
    if (parent.is_valid()) {
      const auto& parent_scope = store->at(parent);
      return parent_scope.lookup_local_function(store, name);
    } else {
      return FunctionReferenceHandle();
    }
  } else {
    return it->second;
  }
}

void MatlabScope::register_local_variable(int64_t name, const VariableDefHandle& handle) {
  local_variables[name] = handle;
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

std::size_t MatlabIdentifier::Hash::operator()(const MatlabIdentifier& k) const {
  using std::hash;
  return hash<int64_t>()(k.name);
}

}