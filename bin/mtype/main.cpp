#include "mt/mt.hpp"
#include "app.hpp"
#include <chrono>

int main(int argc, char** argv) {
  using namespace mt;
  const auto full_t0 = std::chrono::high_resolution_clock::now();

  cmd::Arguments arguments;
  arguments.parse(argc, argv);

  if (arguments.had_parse_error) {
    return -1;

  } else if (arguments.show_help_text) {
    arguments.show_help();
    return 0;

  } else if (arguments.root_identifiers.empty()) {
    arguments.show_usage();
    return 0;
  }

  Optional<SearchPath> maybe_search_path;
  if (arguments.use_search_path_file) {
    maybe_search_path = build_search_path_from_path_file(arguments.search_path_file_path);
  } else {
    maybe_search_path = build_search_path_from_paths(arguments.search_paths);
  }

  if (!maybe_search_path) {
    std::cout << "Failed to build search path." << std::endl;
    return -1;
  }

  App app(arguments, std::move(maybe_search_path.value()));

  bool lookup_success = app.locate_root_identifiers();
  if (!lookup_success) {
    return -1;
  }

  auto& external_functions = app.external_functions;
  auto external_it = external_functions.visited_candidates.begin();

  while (external_it != external_functions.visited_candidates.end()) {
    const auto candidate = external_it++;
    const auto& file_path = candidate->resolved_file->defining_file;

    bool should_reset = app.visit_file(file_path);
    if (should_reset) {
      external_it = external_functions.visited_candidates.begin();
    }
  }

  app.check_for_concrete_function_types();

  const auto full_t1 = std::chrono::high_resolution_clock::now();
  const auto full_elapsed_ms = std::chrono::duration<double>(full_t1 - full_t0).count() * 1e3;
//  const auto check_elapsed_ms = std::chrono::duration<double>(full_t1 - check_t0).count() * 1e3;
//  const auto build_search_path_elapsed_ms =
//    std::chrono::duration<double>(path_t1 - full_t0).count() * 1e3;

  app.maybe_show_type_distribution();
  app.maybe_show_local_variable_types();
  app.maybe_show_local_function_types();
  app.maybe_show_visited_external_files();
  app.maybe_show_errors();
  app.maybe_show_diagnostics();

  return 0;
}