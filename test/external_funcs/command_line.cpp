#include "command_line.hpp"
#include <cstring>
#include <iostream>
#include <string>

namespace mt::cmd {

namespace {
  bool is_argument(const char* a) {
    return std::strlen(a) > 0 && a[0] == '-';
  }

  bool matches(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
  }

  bool matches_path_file(const char* arg) {
    return matches(arg, "--path-file") || matches(arg, "-pf");
  }

  bool matches_store_capacity(const char* arg) {
    return matches(arg, "--store-capacity") || matches(arg, "-sc");
  }

  bool matches_show_ast(const char* arg) {
    return matches(arg, "--show-ast") || matches(arg, "-sa");
  }

  bool matches_show_variable_types(const char* arg) {
    return matches(arg, "--show-var-types") || matches(arg, "-sv");
  }

  bool matches_hide_local_function_types(const char* arg) {
    return matches(arg, "--hide-function-types") || matches(arg, "-hf");
  }

  bool matches_dont_traverse_depends(const char* arg) {
    return matches(arg, "--no-traverse") || matches(arg, "-nt");
  }

  bool matches_hide_diagnostics(const char* arg) {
    return matches(arg, "--hide-diagnostics") || matches(arg, "-hd");
  }

  Optional<int> parse_int(const char* arg) {
    try {
      return Optional<int>(std::stoi(arg));
    } catch (...) {
      return NullOpt{};
    }
  }
}

Arguments::Arguments() {
  search_path_file_path = fs::join(FilePath(MT_MATLAB_DATA_DIR), FilePath("path.txt"));
}

void Arguments::parse(int argc, char** argv) {
  int i = 1;  //  skip executable.
  const int end_less1 = argc - 1;

  while (i < argc) {
    int incr = 1;
    const char* arg = argv[i];

    if (i < end_less1 && matches_path_file(arg)) {
      search_path_file_path = FilePath(argv[i + 1]);
      incr++;

    } else if (i < end_less1 && matches_store_capacity(arg)) {
      const auto maybe_int = parse_int(argv[i + 1]);
      incr++;

      if (maybe_int && maybe_int.value() >= 0) {
        initial_store_capacity = maybe_int.value();
      } else {
        std::cout << "Invalid store capacity." << std::endl;
        had_parse_error = true;
      }

    } else if (matches_show_ast(arg)) {
      show_ast = true;

    } else if (matches_show_variable_types(arg)) {
      show_local_variable_types = true;

    } else if (matches_hide_local_function_types(arg)) {
      show_local_function_types = false;

    } else if (matches_dont_traverse_depends(arg)) {
      traverse_dependencies = false;

    } else if (matches_hide_diagnostics(arg)) {
      show_diagnostics = false;

    } else if (is_argument(arg)) {
      std::cout << "Unrecognized or invalid argument: " << arg << std::endl;
      had_parse_error = true;

    } else {
      root_identifiers.emplace_back(arg);
    }

    i += incr;
  }
}

}
