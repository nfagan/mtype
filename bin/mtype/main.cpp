#include "mt/mt.hpp"
#include "app.hpp"
#include <chrono>

using namespace mt;

namespace {
  Optional<SearchPath> get_search_path(const cmd::Arguments& args) {
    if (args.use_search_path_file) {
      return build_search_path_from_path_file(args.search_path_file_path);
    } else {
      return build_search_path_from_paths(args.search_paths);
    }
  }
}

int main(int argc, char** argv) {
  cmd::Arguments arguments;
  const bool proceed = arguments.parse(argc, argv);
  if (!proceed) {
    return 0;
  }

  auto maybe_search_path = get_search_path(arguments);
  if (!maybe_search_path) {
    std::cout << "Failed to build search path." << std::endl;
    return 0;
  }

  App app(arguments, std::move(maybe_search_path.value()));

  bool lookup_success = app.locate_root_identifiers();
  if (!lookup_success) {
    return 0;
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

  //  No more files are pending, so we should expect each function
  //  in each visited file to have a concrete type.
  app.check_for_concrete_function_types();

  //  Display results.
  app.maybe_show();

  return 0;
}