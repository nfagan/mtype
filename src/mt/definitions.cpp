#include "definitions.hpp"
#include "ast/ast.hpp"

namespace mt {

std::size_t MatlabIdentifier::Hash::operator()(const MatlabIdentifier& k) const {
  using std::hash;
  return hash<int64_t>()(k.name);
}

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

void MatlabScope::register_local_variable(int64_t name, const VariableDefHandle& handle) {
  local_variables[name] = handle;
}

}
