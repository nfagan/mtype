#include "definitions.hpp"
#include "keyword.hpp"
#include <cassert>

namespace mt {

std::size_t FunctionParameter::Hash::operator()(const FunctionParameter& p) const noexcept {
  return MatlabIdentifier::Hash{}(p.name) ^ std::hash<AttributeFlags::FlagType>{}(p.flags);
}

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

/*
 * FunctionHeader
 */

int64_t FunctionHeader::num_inputs() const {
  return inputs.size();
}

int64_t FunctionHeader::num_outputs() const {
  return outputs.size();
}

FunctionHeader::UniqueParameters FunctionHeader::unique_parameters() const {
  std::unordered_set<FunctionParameter, FunctionParameter::Hash> result;

  for (const auto& input : inputs) {
    result.insert(input);
  }
  for (const auto& output: outputs) {
    result.insert(output);
  }

  return result;
}

/*
 * MatlabScope
 */

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

#define MT_SCOPE_LOOKUP(member, null_result_type) \
  const MatlabScope* scope = this; \
  while (scope) { \
    const auto it = scope->member.find(name); \
    if (it == scope->member.end()) { \
      scope = scope->parent; \
    } else { \
      return it->second; \
    } \
  } \
  return null_result_type();

FunctionReferenceHandle MatlabScope::lookup_local_function(const MatlabIdentifier& name) const {
  MT_SCOPE_LOOKUP(local_functions, FunctionReferenceHandle)
}

FunctionReferenceHandle MatlabScope::lookup_imported_function(const MatlabIdentifier& name) const {
  MT_SCOPE_LOOKUP(imported_functions, FunctionReferenceHandle)
}

VariableDefHandle MatlabScope::lookup_local_variable(const MatlabIdentifier& name) const {
  MT_SCOPE_LOOKUP(local_variables, VariableDefHandle)
}

bool MatlabScope::has_imported_function(const MatlabIdentifier& name) const {
  return lookup_imported_function(name).is_valid();
}

bool MatlabScope::has_local_variable(const MatlabIdentifier& name) const {
  return local_variables.count(name) > 0;
}

/*
 * ClassDef
 */

}
