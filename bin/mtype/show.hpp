#pragma once

#include "mt/mt.hpp"

namespace mt {

class AstStore;

namespace cmd {
  struct Arguments;
}

template <typename... Args>
TypeToString make_type_to_string(Args&&... args);

void configure_type_to_string(TypeToString& type_to_string, const cmd::Arguments& args);

void show_parse_errors(const ParseErrors& errors,
                       const TokenSourceMap& source_data,
                       const cmd::Arguments& arguments);

template <typename T>
void show_type_errors(const std::vector<T>& errors,
                      const TokenSourceMap& source_data,
                      const TypeToString& type_to_string);

void show_function_types(const FunctionsByFile& functions_by_file,
                         const TypeToString& type_to_string,
                         const Store& store,
                         const StringRegistry& string_registry,
                         const Library& library);

void show_asts(const AstStore& ast_store,
               const Store& def_store,
               const StringRegistry& string_registry,
               const cmd::Arguments& arguments);
}

/*
 * impl
 */

namespace mt {

template <typename... Args>
TypeToString make_type_to_string(Args&&... args) {
  TypeToString type_to_string(std::forward<Args>(args)...);
  return type_to_string;
}

template <typename T>
void show_type_errors(const std::vector<T>& errors,
                      const TokenSourceMap& source_data,
                      const TypeToString& type_to_string) {
  ShowTypeErrors show(type_to_string);
  show.show(errors, source_data);
}

}