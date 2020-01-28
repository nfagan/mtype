#pragma once

#include "ast.hpp"
#include "lang_components.hpp"

namespace mt {
class ExternalFunctionRegistry {
public:
  ExternalFunctionRegistry() = default;
  ~ExternalFunctionRegistry() = default;

  FunctionReference* make_reference(int64_t to_identifier, std::shared_ptr<MatlabScope> in_scope);

private:
  std::vector<std::unique_ptr<FunctionReference>> functions;
};
}