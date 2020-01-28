#include "ExternalFunctionRegistry.hpp"

namespace mt {

FunctionReference* ExternalFunctionRegistry::make_reference(int64_t to_identifier, std::shared_ptr<MatlabScope> in_scope) {
  auto ref = std::make_unique<FunctionReference>(to_identifier, nullptr, std::move(in_scope));
  auto ptr = ref.get();
  functions.emplace_back(std::move(ref));
  return ptr;
}

}
