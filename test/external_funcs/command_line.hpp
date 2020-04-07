#pragma once

#include "mt/mt.hpp"

namespace mt::cmd {
struct Arguments {
  Arguments();
  void parse(int argc, char** argv);

  FilePath search_path_file_path;
  std::vector<std::string> root_identifiers;

  bool show_ast = false;
  bool show_local_function_types = true;
  bool show_local_variable_types = false;
  bool traverse_dependencies = true;
  bool show_diagnostics = true;

  bool had_unrecognized_argument = false;
};
}