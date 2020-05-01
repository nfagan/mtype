#pragma once

#include "mt/mt.hpp"

namespace mt::cmd {

struct MatchResult {
  bool success;
  int increment;
};

using MatchCallback = std::function<MatchResult(int, int, char**)>;

struct ParameterName {
  ParameterName();
  ParameterName(const char* full, const char* alias);
  ParameterName(const char* single);

  bool matches(const char* arg) const;
  std::string to_string() const;

  std::array<const char*, 2> alternates;
  int num_alternates;
};

struct Argument {
  Argument(const ParameterName& param, std::string description, MatchCallback cb);
  Argument(const ParameterName& param, const ParameterName& args,
           std::string description, MatchCallback cb);

  std::string to_string() const;

  ParameterName param;
  Optional<ParameterName> arguments;
  std::string description;
  MatchCallback match_callback;
};

struct Arguments {
  Arguments();
  bool parse(int argc, char** argv);
  void show_help() const;
  void show_usage() const;

private:
  bool evaluate() const;
  void build_parse_spec();
  void make_silent();

private:
  std::vector<Argument> arguments;

public:
  FilePath search_path_file_path;
  std::vector<std::string> root_identifiers;
  std::vector<mt::FilePath> search_paths;
  std::vector<std::string> pre_imports;
  std::vector<std::string> error_filter_identifiers;

  bool show_ast = false;
  bool show_local_variable_types = false;
  bool traverse_dependencies = true;

#if MT_DEBUG
  bool show_diagnostics = true;
  bool show_local_function_types = true;
#else
  bool show_diagnostics = false;
  bool show_local_function_types = false;
#endif

  bool show_class_source_type = false;
  bool use_search_path_file = true;
  bool rich_text = true;
  bool show_errors = true;
  bool show_warnings = true;
  bool show_type_distribution = false;
  bool show_help_text = false;
  bool show_visited_external_files = false;
  bool show_explicit_destructured_tuples = false;
  bool show_explicit_aliases = false;
  bool use_arrow_function_notation = false;
  bool show_application_outputs = false;

  bool had_parse_error = false;
  int initial_store_capacity = 100000;
  int max_num_type_variables = 3;
};
}