#include "definitions.hpp"
#include "keyword.hpp"
#include <cassert>

namespace mt {

FunctionAttributes FunctionAttributes::extract_methods_block_attributes(const FunctionAttributes& attribs) {
  auto res = attribs;
  res.boolean_attributes &= (~AttributeFlags::is_constructor);
  res.boolean_attributes &= (~AttributeFlags::has_varargin);
  res.boolean_attributes &= (~AttributeFlags::has_varargout);
  return res;
}

void FunctionAttributes::mark_boolean_attribute_from_name(std::string_view name) {
  if (matlab::is_abstract_attribute(name)) {
    mark_abstract();

  } else if (matlab::is_hidden_attribute(name)) {
    mark_hidden();

  } else if (matlab::is_sealed_attribute(name)) {
    mark_sealed();

  } else if (matlab::is_static_attribute(name)) {
    mark_static();

  } else {
    //  Should be handled by a preceding call to is_method_attribute(name).
    assert(false && "Unhandled method attribute name.");
  }
}

void MatlabScope::register_import(Import&& import) {
  if (import.type == ImportType::wildcard) {
    wildcard_imports.emplace_back(import);
  } else {
    fully_qualified_imports.emplace_back(import);
  }
}

void MatlabScope::register_imported_function(const MatlabIdentifier& name, const FunctionReferenceHandle& handle) {
  imported_functions[name] = handle;
}

bool MatlabScope::register_class(const MatlabIdentifier& name, const ClassDefHandle& handle) {
  if (classes.count(name) > 0) {
    return false;
  } else {
    classes[name] = handle;
    return true;
  }
}

bool MatlabScope::register_local_function(const MatlabIdentifier& name, const FunctionReferenceHandle& ref) {
  if (local_functions.count(name) > 0) {
    return false;
  } else {
    local_functions[name] = ref;
    return true;
  }
}

void MatlabScope::register_local_variable(const MatlabIdentifier& name, const VariableDefHandle& handle) {
  local_variables[name] = handle;
}

FunctionReferenceHandle MatlabScope::lookup_local_function(const MatlabIdentifier& name) const {
  const MatlabScope* scope = this;

  while (scope) {
    const auto it = scope->local_functions.find(name);
    if (it == scope->local_functions.end()) {
      scope = scope->parent;
    } else {
      return it->second;
    }
  }

  return FunctionReferenceHandle();
}

FunctionReferenceHandle MatlabScope::lookup_imported_function(const MatlabIdentifier& name) const {
  const MatlabScope* scope = this;

  while (scope) {
    const auto it = scope->imported_functions.find(name);
    if (it == scope->imported_functions.end()) {
      scope = scope->parent;
    } else {
      return it->second;
    }
  }

  return FunctionReferenceHandle();
}

bool MatlabScope::has_imported_function(const MatlabIdentifier& name) const {
  return lookup_imported_function(name).is_valid();
}

/*
 * ClassDef
 */

}
