#include "ast.hpp"
#include "def.hpp"
#include "StringVisitor.hpp"
#include "../identifier_classification.hpp"

namespace mt {

void MatlabScope::register_import(Import&& import) {
  if (import.type == ImportType::wildcard) {
    wildcard_imports.emplace_back(import);
  } else {
    fully_qualified_imports.emplace_back(import);
  }
}

bool MatlabScope::register_local_function(int64_t name, FunctionReference* ref) {
  if (local_functions.count(name) > 0) {
    return false;
  } else {
    local_functions[name] = ref;
    return true;
  }
}

namespace {
namespace detail {
  template <typename Result, typename Scope>
  inline Result* lookup_local_function_impl(Scope* scope, int64_t name) {
    auto it = scope->local_functions.find(name);
    if (it == scope->local_functions.end()) {
      return scope->parent ? scope->parent->lookup_local_function(name) : nullptr;
    } else {
      return it->second;
    }
  }
}
}

const FunctionReference* MatlabScope::lookup_local_function(int64_t name) const {
  return detail::lookup_local_function_impl<const FunctionReference, const MatlabScope>(this, name);
}

FunctionReference* MatlabScope::lookup_local_function(int64_t name) {
  return detail::lookup_local_function_impl<FunctionReference, MatlabScope>(this, name);
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

ClassDefReference* ClassDefReference::accept(IdentifierClassifier& classifier) {
  return classifier.class_def_reference(*this);
}

std::string ClassDefReference::accept(const StringVisitor& vis) const {
  return vis.class_def_reference(*this);
}

}