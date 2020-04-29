#include "command_line.hpp"
#include <cstring>
#include <iostream>
#include <string>
#include <functional>

namespace mt::cmd {

namespace {
  bool matches(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
  }

  bool is_argument(const char* a) {
    return std::strlen(a) > 0 && a[0] == '-';
  }

#if 0
  Optional<int> parse_int(const char* arg) {
    try {
      return Optional<int>(std::stoi(arg));
    } catch (...) {
      return NullOpt{};
    }
  }
#endif

  std::vector<FilePath> get_split_paths(const std::string& arg) {
    auto split = mt::split(arg.c_str(), arg.size(), Character(':'));
    std::vector<FilePath> result;
    result.reserve(split.size());
    for (const auto& s : split) {
      result.emplace_back(std::string(s));
    }
    return result;
  }
}

/*
 * ParameterName
 */

ParameterName::ParameterName() : num_alternates(0) {
  //
}
ParameterName::ParameterName(const char* full, const char* alias) :
alternates{{full, alias}}, num_alternates(2) {
  //
}
ParameterName::ParameterName(const char* single) :
alternates{{single}}, num_alternates(1) {
  //
}

bool ParameterName::matches(const char* arg) const {
  for (int i = 0; i < num_alternates; i++) {
    if (mt::cmd::matches(arg, alternates[i])) {
      return true;
    }
  }
  return false;
}

std::string ParameterName::to_string() const {
  std::string res;
  for (int i = 0; i < num_alternates; i++) {
    res += alternates[i];
    if (i < num_alternates-1) {
      res += ", ";
    }
  }
  return res;
}

/*
 * Argument
 */

Argument::Argument(const ParameterName& param, std::string description, MatchCallback cb) :
  param(param), description(std::move(description)), match_callback(std::move(cb)) {
  //
}
Argument::Argument(const ParameterName& param, const ParameterName& args, std::string description, MatchCallback cb) :
  param(param), arguments(Optional<ParameterName>(args)), description(std::move(description)), match_callback(std::move(cb)) {
  //
}

std::string Argument::to_string() const {
  std::string str = "  " + param.to_string();
  if (arguments) {
    str += " ";
    str += arguments.value().to_string();
  }

  str += ": ";
  str += "\n      " + description;

  return str;
}

namespace {
  MatchResult true_param(bool* out) {
    *out = true;
    return MatchResult{true, 1};
  }
  MatchResult false_param(bool* out) {
    *out = false;
    return MatchResult{true, 1};
  }
}

/*
 * Arguments
 */

Arguments::Arguments() {
  search_path_file_path = fs::join(FilePath(MT_MATLAB_DATA_DIR), FilePath("path.txt"));
}

void Arguments::show_help() const {
  std::cout << std::endl;
  show_usage();
  std::cout << std::endl << "options: " << std::endl;

  for (const auto& arg : arguments) {
    std::cout << arg.to_string() << std::endl;
  }

  std::cout << std::endl;
}

void Arguments::show_usage() const {
  std::cout << "Usage: mtype [options] file-name [file-name2 ...]" << std::endl;
}

void Arguments::build_parse_spec() {
  arguments.emplace_back(ParameterName("--help", "-h"), "Show this text.", [this](int, int, char**) {
    return true_param(&show_help_text);
  });
  arguments.emplace_back(ParameterName("--show-ast", "-sa"), "Print each file's AST.",
    [this](int, int, char**) {
    return true_param(&show_ast);
  });
  arguments.emplace_back(ParameterName("--show-var-types", "-sv"), "Print the types of local variables.",
    [this](int, int, char**) {
    return true_param(&show_local_variable_types);
  });
  arguments.emplace_back(ParameterName("--matlab-function-types", "-mft"), "Print function types using the form: [] = ().",
    [this](int, int, char**) {
     return false_param(&use_arrow_function_notation);
  });
  arguments.emplace_back(ParameterName("--arrow-function-types", "-aft"), "Print function types using the form: () -> [].",
    [this](int, int, char**) {
    return true_param(&use_arrow_function_notation);
  });
  arguments.emplace_back(ParameterName("--show-visited-files", "-svf"), "Print the paths of traversed files.",
    [this](int, int, char**) {
    return true_param(&show_visited_external_files);
  });
  arguments.emplace_back(ParameterName("--show-class-source", "-scs"), "Print the source type for class types.",
    [this](int, int, char**) {
    return true_param(&show_class_source_type);
  });
  arguments.emplace_back(ParameterName("--show-dist", "-sd"), "Print the distribution of types.",
    [this](int, int, char**) {
    return true_param(&show_type_distribution);
  });
  arguments.emplace_back(ParameterName("--explicit-dt", "-edt"), "Print destructured tuples explicitly.",
    [this](int, int, char**) {
    return true_param(&show_explicit_destructured_tuples);
  });
  arguments.emplace_back(ParameterName("--explicit-aliases", "-ea"), "Print aliases explicitly.",
    [this](int, int, char**) {
    return true_param(&show_explicit_aliases);
  });
  arguments.emplace_back(ParameterName("--plain-text", "-pt"), "Use ASCII text and no control characters when printing.",
    [this](int, int, char**) {
    return false_param(&rich_text);
  });
  arguments.emplace_back(ParameterName("--hide-function-types", "-hf"), "Don't print local function types.",
    [this](int, int, char**) {
    return false_param(&show_local_function_types);
  });
  arguments.emplace_back(ParameterName("--show-function-types", "-sf"), "Print local function types.",
    [this](int, int, char**) {
    return true_param(&show_local_function_types);
  });
  arguments.emplace_back(ParameterName("--hide-diagnostics", "-hdi"), "Don't print elapsed time and other diagnostics.",
    [this](int, int, char**) {
    return false_param(&show_diagnostics);
  });
  arguments.emplace_back(ParameterName("--show-diagnostics", "-sdi"), "Print elapsed time and other diagnostics.",
    [this](int, int, char**) {
    return true_param(&show_diagnostics);
  });
  arguments.emplace_back(ParameterName("--hide-errors", "-he"), "Don't print errors.",
    [this](int, int, char**) {
    return false_param(&show_errors);
  });
  arguments.emplace_back(ParameterName("--hide-warnings", "-hw"), "Don't print warnings.",
    [this](int, int, char**) {
    return false_param(&show_warnings);
  });
  arguments.emplace_back(ParameterName("--path", "-p"), "`str`",
    "Use directories in the ':'-delimited `str` to build the search path.",
    [this](int i, int argc, char** argv) {
    if (i >= argc-1) {
      return MatchResult{false, 1};
    } else {
      use_search_path_file = false;
      search_paths = get_split_paths(argv[i + 1]);
      return MatchResult{true, 2};
    }
  });
  arguments.emplace_back(ParameterName("--path-file", "-pf"), "`file`",
    "Use the newline-delimited directories in `file` to build the search path.",
    [this](int i, int argc, char** argv) {
    if (i >= argc-1) {
      return MatchResult{false, 1};
    } else {
      search_path_file_path = FilePath(argv[i + 1]);
      return MatchResult{true, 2};
    }
  });
}

void Arguments::parse(int argc, char** argv) {
  build_parse_spec();

  int i = 1;  //  skip executable.

  while (i < argc) {
    int incr = 1;
    const char* arg = argv[i];
    const bool is_arg = is_argument(arg);
    bool any_matched = false;
    bool parse_success = true;

    for (const auto& to_match : arguments) {
      if (to_match.param.matches(arg)) {
        auto res = to_match.match_callback(i, argc, argv);
        incr = res.increment;
        any_matched = true;
        had_parse_error = had_parse_error || !res.success;
        parse_success = res.success;
        break;
      }
    }

    if (!any_matched && is_arg) {
      std::cout << "Unrecognized or invalid argument: " << arg << ".";
      std::cout << " Try mtype --help." << std::endl;
      had_parse_error = true;

    } else if (!is_arg) {
      root_identifiers.emplace_back(arg);

    } else if (!parse_success) {
      std::cout << "Invalid value for argument: " << arg << ".";
      std::cout << " Try mtype --help." << std::endl;
    }

    i += incr;
  }
}

}
