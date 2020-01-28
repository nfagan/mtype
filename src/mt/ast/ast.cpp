#include "ast.hpp"
#include "def.hpp"

namespace mt {

bool MatlabScope::register_local_function(int64_t name, FunctionDef* def) {
  if (local_functions.count(name) > 0) {
    return false;
  } else {
    local_functions[name] = def;
    return true;
  }
}

void MatlabScope::register_local_variable(int64_t name, std::unique_ptr<VariableDef> def) {
  local_variables[name] = std::move(def);
}

}