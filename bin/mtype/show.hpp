#pragma once

#include "mt/mt.hpp"

namespace mt {

namespace cmd {
  struct Arguments;
}

template <typename... Args>
TypeToString make_type_to_string(Args&&... args) {
  TypeToString type_to_string(std::forward<Args>(args)...);
  return type_to_string;
}

void configure_type_to_string(TypeToString& type_to_string, const cmd::Arguments& args);

void show_parse_errors(const ParseErrors& errors,
                       const TokenSourceMap& source_data,
                       const cmd::Arguments& arguments);

void show_type_errors(const TypeErrors& errors,
                      const TokenSourceMap& source_data,
                      const TypeToString& type_to_string,
                      const cmd::Arguments& arguments);

void show_function_types(const FunctionsByFile& functions_by_file,
                         const TypeToString& type_to_string,
                         const Store& store,
                         const StringRegistry& string_registry,
                         const Library& library);
}