#pragma once

#include "mt/mt.hpp"

namespace mt {

class AstStore;

struct ConcreteFunctionTypeInstance {
  ConcreteFunctionTypeInstance(const Library& library,
                               const Store& def_store,
                               const StringRegistry& string_registry);

  const Library& library;
  const Store& def_store;
  const StringRegistry& string_registry;

  std::vector<BoxedTypeError> errors;
};

void check_for_concrete_function_type(ConcreteFunctionTypeInstance& instance,
                                      const FunctionDefHandle& def_handle,
                                      const Type* source_type);

bool can_show_untyped_function_errors(const Store& store,
                                      const AstStore& ast_store,
                                      const FunctionDefHandle& def_handle);
}