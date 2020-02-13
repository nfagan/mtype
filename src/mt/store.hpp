#pragma once

#include "ast.hpp"
#include "lang_components.hpp"

namespace mt {
class FunctionRegistry {
public:
  FunctionRegistry() = default;
  ~FunctionRegistry() = default;

  FunctionReference* make_external_reference(int64_t to_identifier, std::shared_ptr<MatlabScope> in_scope);
  void emplace_local_definition(std::unique_ptr<FunctionDef> function_def);

private:
  std::vector<std::unique_ptr<FunctionReference>> external_function_references;
  std::vector<std::unique_ptr<FunctionDef>> local_function_definitions;
};

class ClassDefStore {
public:
  ClassDefStore() = default;
  ~ClassDefStore() = default;

  ClassDefHandle emplace_definition(ClassDef&& def);
  const ClassDef& lookup_class(const ClassDefHandle& by_handle) const;
  ClassDef& lookup_class(ClassDefHandle& by_handle);

private:
  std::vector<ClassDef> definitions;
};

}