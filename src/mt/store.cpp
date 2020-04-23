#include "store.hpp"
#include <cassert>

namespace mt {

/*
 * Class components
 */

ClassDefHandle Store::make_class_definition() {
  class_definitions.emplace_back();
  return ClassDefHandle(class_definitions.size() - 1);
}

FunctionDefHandle Store::make_function_declaration(FunctionHeader&& header, const FunctionAttributes& attrs) {
  function_definitions.emplace_back(std::move(header), attrs);
  return FunctionDefHandle(function_definitions.size() - 1);
}

void Store::emplace_definition(const ClassDefHandle& at_handle, ClassDef&& def) {
  class_definitions[at_handle.index] = std::move(def);
}

const ClassDef& Store::at(const ClassDefHandle& handle) const {
  return class_definitions[handle.index];
}

ClassDef& Store::at(const ClassDefHandle& handle) {
  return class_definitions[handle.index];
}

Optional<FunctionDefHandle> Store::extract_constructor(const ClassDefHandle& for_class) const {
  const auto& class_def = at(for_class);
  const auto& methods = class_def.methods;

  for (const auto& method : methods) {
    FunctionDefHandle maybe_ctor_handle;

    const auto& def = at(method);
    if (def.attributes.is_constructor()) {
      maybe_ctor_handle = method;
    }

    if (maybe_ctor_handle.is_valid()) {
      return Optional<FunctionDefHandle>(maybe_ctor_handle);
    }
  }

  return NullOpt{};
}

/*
 * Variable components
 */

VariableDefHandle Store::emplace_definition(mt::VariableDef&& def) {
  variable_definitions.emplace_back(std::move(def));
  return VariableDefHandle(variable_definitions.size() - 1);
}

VariableDefHandle Store::make_variable_def(VariableDef&& def) {
  Store::Write write(*this);
  return emplace_definition(std::move(def));
}

const VariableDef& Store::at(const mt::VariableDefHandle& handle) const {
  return variable_definitions[handle.index];
}

VariableDef& Store::at(const VariableDefHandle& handle) {
  return variable_definitions[handle.index];
}

/*
 * Function components
 */

FunctionDef& Store::at(const FunctionDefHandle& handle) {
  return function_definitions[handle.index];
}

const FunctionDef& Store::at(const FunctionDefHandle& handle) const {
  return function_definitions[handle.index];
}

const FunctionReference& Store::at(const FunctionReferenceHandle& handle) const {
  return function_references[handle.index];
}

FunctionReference Store::get(const FunctionReferenceHandle& handle) const {
  assert(handle.is_valid() && handle.index < int64_t(function_references.size()));
  Store::ReadConst reader(*this);
  return function_references[handle.index];
}

Block* Store::get_block(const FunctionDefHandle& handle) const {
  Store::ReadConst reader(*this);
  return reader.at(handle).body.get();
}

MatlabIdentifier Store::get_name(const ClassDefHandle& handle) const {
  Store::ReadConst reader(*this);
  return reader.at(handle).name;
}

MatlabIdentifier Store::get_name(const FunctionDefHandle& handle) const {
  Store::ReadConst reader(*this);
  return reader.at(handle).header.name;
}

FunctionDefHandle Store::emplace_definition(FunctionDef&& def) {
  function_definitions.emplace_back(std::move(def));
  return FunctionDefHandle(function_definitions.size() - 1);
}

FunctionReferenceHandle Store::make_external_reference(const MatlabIdentifier& to_identifier,
                                                       const MatlabScope* in_scope) {
  return make_local_reference(to_identifier, FunctionDefHandle(), in_scope);
}

FunctionReferenceHandle Store::make_local_reference(const MatlabIdentifier& to_identifier,
                                                    const FunctionDefHandle& with_def,
                                                    const MatlabScope* in_scope) {
  FunctionReference reference(to_identifier, with_def, in_scope);
  function_references.emplace_back(reference);
  return FunctionReferenceHandle(function_references.size() - 1);
}

/*
 * Scope components
 */

MatlabScope* Store::make_matlab_scope(const MatlabScope* parent, const CodeFileDescriptor* file_descriptor) {
  auto new_scope = std::make_unique<MatlabScope>(parent, file_descriptor);
  auto scope_ptr = new_scope.get();
  matlab_scopes.push_back(std::move(new_scope));
  return scope_ptr;
}

TypeScope* Store::make_type_scope(TypeScope* root, const TypeScope* parent) {
  auto new_scope = std::make_unique<TypeScope>(root, parent);
  auto scope_ptr = new_scope.get();
  type_scopes.push_back(std::move(new_scope));
  return scope_ptr;
}

}
