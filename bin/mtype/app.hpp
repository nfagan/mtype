#pragma once

#include "mt/mt.hpp"
#include "ast_store.hpp"
#include "parse_pipeline.hpp"
#include "command_line.hpp"
#include "external_resolution.hpp"

namespace mt {

namespace cmd {
  struct Arguments;
}

class App {
public:
  App(const cmd::Arguments& args, SearchPath&& search_path);

  bool visit_file(const FilePath& file_path);
  bool locate_root_identifiers();
  void check_for_concrete_function_types();

  void maybe_show_local_function_types() const;
  void maybe_show_local_variable_types() const;
  void maybe_show_visited_external_files() const;
  void maybe_show_errors() const;
  void maybe_show_diagnostics() const;
  void maybe_show_type_distribution() const;
  void maybe_show_asts() const;

private:
  bool add_base_scopes(const AstStoreEntries& entries) const;
  bool resolve_type_imports(AstStoreEntryPtr root_entry);
  bool resolve_type_identifiers(const AstStoreEntries& entries);
  bool resolve_external_functions(ParsePipelineInstanceData& pipeline_instance);
  bool generate_type_constraints(const AstStoreEntries& entries);

  void initialize();
  void unify();
  void add_root_identifier(const std::string& name,
                           const SearchCandidate* source_candidate);

public:
  cmd::Arguments arguments;

  SearchPath search_path;
  Store store;
  TypeStore type_store;
  StringRegistry string_registry;
  TokenSourceMap source_data_by_token;
  Library library;
  Substitution substitution;
  Unifier unifier;
  TypeConstraintGenerator constraint_generator;
  PendingExternalFunctions external_functions;

  TypeToString type_to_string;

  AstStore ast_store;
  ScanResultStore scan_result_store;
  FunctionsByFile functions_by_file;
  VisitedResolutionPairs visited_resolution_pairs;
  std::vector<std::unique_ptr<Token>> temporary_source_tokens;

  ParseErrors parse_errors;
  ParseErrors parse_warnings;

  TypeErrors type_errors;
};

}