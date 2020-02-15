#pragma once

#include "ast.hpp"
#include "lang_components.hpp"

namespace mt {

class VariableStore {
public:
  VariableStore() = default;
  ~VariableStore() = default;

  VariableDefHandle emplace_definition(VariableDef&& def);
  const VariableDef& at(const VariableDefHandle& handle) const;
  VariableDef& at(const VariableDefHandle& handle);

private:
  std::vector<VariableDef> definitions;
};

class FunctionStore {
public:
  FunctionStore() = default;
  ~FunctionStore() = default;

  FunctionDefHandle emplace_definition(FunctionDef&& def);
  FunctionReferenceHandle make_external_reference(int64_t to_identifier, BoxedMatlabScope in_scope);
  FunctionReferenceHandle make_local_reference(int64_t to_identifier, FunctionDefHandle with_def,
    BoxedMatlabScope in_scope);

  const FunctionDef& at(const FunctionDefHandle& handle) const;
  FunctionDef& at(const FunctionDefHandle& handle);

  const FunctionReference& at(const FunctionReferenceHandle& handle) const;

private:
  std::vector<FunctionDef> definitions;
  std::vector<FunctionReference> references;
};

class ClassStore {
public:
  ClassStore() = default;
  ~ClassStore() = default;

  ClassDefHandle emplace_definition(ClassDef&& def);
  const ClassDef& at(const ClassDefHandle& by_handle) const;
  ClassDef& at(ClassDefHandle& by_handle);

private:
  std::vector<ClassDef> definitions;
};

}