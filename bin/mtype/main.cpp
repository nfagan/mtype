#include "mt/mt.hpp"
#include "command_line.hpp"
#include "ast_store.hpp"
#include "parse_pipeline.hpp"
#include "external_resolution.hpp"
#include "type_analysis.hpp"
#include <chrono>

namespace {

template <typename T>
void move_from(std::vector<T>& source, std::vector<T>& dest) {
  std::move(source.begin(), source.end(), std::back_inserter(dest));
}

template <typename... Args>
mt::TypeToString make_type_to_string(Args&&... args) {
  mt::TypeToString type_to_string(std::forward<Args>(args)...);
  return type_to_string;
}

void configure_type_to_string(mt::TypeToString& type_to_string, const mt::cmd::Arguments& args) {
  type_to_string.explicit_destructured_tuples = args.show_explicit_destructured_tuples;
  type_to_string.explicit_aliases = args.show_explicit_aliases;
  type_to_string.arrow_function_notation = args.use_arrow_function_notation;
  type_to_string.max_num_type_variables = args.max_num_type_variables;
  type_to_string.show_class_source_type = args.show_class_source_type;
  type_to_string.rich_text = args.rich_text;
  type_to_string.show_application_outputs = args.show_application_outputs;
}

void show_parse_errors(const mt::ParseErrors& errors,
                       const mt::TokenSourceMap& source_data,
                       const mt::cmd::Arguments& arguments) {
  mt::ShowParseErrors show_errs;
  show_errs.is_rich_text = arguments.rich_text;
  show_errs.show(errors, source_data);
}

void show_type_errors(const mt::TypeErrors& errors,
                      const mt::TokenSourceMap& source_data,
                      const mt::TypeToString& type_to_string,
                      const mt::cmd::Arguments& arguments) {
  mt::ShowTypeErrors show(type_to_string);
  show.show(errors, source_data);
}

void add_root_identifier(const std::string& name,
                         const mt::SearchCandidate* source_candidate,
                         mt::PendingExternalFunctions& external_functions,
                         mt::StringRegistry& string_registry,
                         mt::TypeStore& type_store) {

  mt::MatlabIdentifier ident(string_registry.register_string(name));
  mt::FunctionSearchCandidate search_candidate(source_candidate, ident);
  external_functions.add_visited_candidate(search_candidate);
}

bool locate_root_identifiers(const std::vector<std::string>& names,
                             const mt::SearchPath& search_path,
                             mt::PendingExternalFunctions& external_functions,
                             mt::StringRegistry& string_registry,
                             mt::TypeStore& type_store) {
  for (const auto& name : names) {
    const auto maybe_source_candidate = search_path.search_for(name);
    if (!maybe_source_candidate) {
      std::cout << "No file on the search path matched: " << name << std::endl;
      return false;
    }

    add_root_identifier(name, maybe_source_candidate.value(),
      external_functions, string_registry, type_store);
  }

  return true;
}

void run_unify(mt::Unifier& unifier,
               mt::Substitution& substitution,
               mt::PendingExternalFunctions& external_functions,
               mt::TypeErrors& type_errors,
               double& unify_time) {

  auto t0 = std::chrono::high_resolution_clock::now();
  auto unify_res = unifier.unify(&substitution, &external_functions);
  auto t1 = std::chrono::high_resolution_clock::now();
  unify_time += std::chrono::duration<double>(t1 - t0).count() * 1e3;

  if (unify_res.is_error()) {
    move_from(unify_res.errors, type_errors);
    if (unify_res.errors.empty()) {
      std::cout << "ERROR: An error occurred, but no explicit errors were generated." << std::endl;
    }
  }
}

}

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

  const auto path_t1 = std::chrono::high_resolution_clock::now();
  //  Time excluding time to build the search path
  const auto check_t0 = std::chrono::high_resolution_clock::now();

  const auto& search_path = maybe_search_path.value();

  Store store;
  StringRegistry str_registry;
  TypeStore type_store(arguments.initial_store_capacity);
  Library library(type_store, store, search_path, str_registry);
  library.make_known_types();

  auto type_to_string = make_type_to_string(&library, &str_registry);
  configure_type_to_string(type_to_string, arguments);

  Substitution substitution;
  Unifier unifier(type_store, library, str_registry);
  TypeConstraintGenerator constraint_generator(substitution, store, type_store, library, str_registry);

  PendingExternalFunctions external_functions;
  bool lookup_res = locate_root_identifiers(arguments.root_identifiers, search_path,
                                            external_functions, str_registry, type_store);
  if (!lookup_res) {
    return -1;
  }

  AstStore ast_store;
  ScanResultStore scan_result_store;
  std::vector<std::unique_ptr<Token>> temporary_source_tokens;
  ParseErrors parse_errors;
  ParseErrors parse_warnings;
  TypeErrors type_errors;

  VisitedResolutionPairs visited_pairs;

  TokenSourceMap source_data_by_token;
  double constraint_time = 0;
  double unify_time = 0;

  auto external_it = external_functions.visited_candidates.begin();
  while (external_it != external_functions.visited_candidates.end()) {
    using clock = std::chrono::high_resolution_clock;

    const auto candidate = external_it++;
    const auto& file_path = candidate->resolved_file->defining_file;

    ParsePipelineInstanceData pipeline_instance(search_path, store, type_store, library, str_registry,
                                                ast_store, scan_result_store, source_data_by_token,
                                                arguments, parse_errors, parse_warnings);

    auto root_res = file_entry(pipeline_instance, file_path);
    if (!root_res || !root_res->root_block || root_res->generated_type_constraints) {
      continue;
    }

    for (auto& root : pipeline_instance.roots) {
      root->type_scope->add_import(TypeImport(library.base_scope, false));
    }

    TypeImportResolutionInstance import_res(source_data_by_token);
    auto resolution_result = maybe_resolve_type_imports(root_res->root_block->type_scope, import_res);
    if (!resolution_result.success) {
      parse_errors.insert(parse_errors.end(), import_res.errors.cbegin(), import_res.errors.cend());
      continue;
    }

    //  Type identifier resolution.
    bool any_resolution_errors = false;
    std::vector<TypeIdentifierResolverInstance::PendingScheme> pending_schemes;

    for (const auto& root_file : pipeline_instance.root_files) {
      auto entry = ast_store.lookup(root_file);
      assert(!entry->resolved_type_identifiers);
      if (entry->resolved_type_identifiers) {
        continue;
      }

      TypeIdentifierResolverInstance instance(type_store, library, store, str_registry, source_data_by_token);
      TypeIdentifierResolver type_identifier_resolver(&instance);

      const auto& root = entry->root_block;
      root->accept(type_identifier_resolver);
      entry->resolved_type_identifiers = true;

      if (instance.had_error()) {
        parse_errors.insert(parse_errors.end(), instance.errors.cbegin(), instance.errors.cend());
        any_resolution_errors = true;
      } else {
        //  Push schemes that need instantiation.
        move_from(instance.pending_schemes, pending_schemes);
      }
    }

    if (any_resolution_errors) {
      continue;
    }

    for (const auto& pending_scheme : pending_schemes) {
      pending_scheme.instantiate(type_store);
    }

    //  Type constraint generation.
    for (const auto& root_file : pipeline_instance.root_files) {
      auto entry = ast_store.lookup(root_file);
      if (entry->generated_type_constraints) {
        continue;
      }
      entry->root_block->accept_const(constraint_generator);
      entry->generated_type_constraints = true;
    }

    auto& gen_warnings = constraint_generator.get_warnings();
    move_from(gen_warnings, type_errors);

    run_unify(unifier, substitution, external_functions, type_errors, unify_time);

    ResolutionInstance resolution_instance;
    auto resolution_pairs =
      resolve_external_functions(resolution_instance, pipeline_instance,
                                 external_functions, visited_pairs);

    for (const auto& res_pair : resolution_pairs) {
      const auto& source_token = res_pair.source_token;
      auto boxed_tok = std::make_unique<Token>(source_token);
      auto tok_ptr = boxed_tok.get();
      temporary_source_tokens.push_back(std::move(boxed_tok));

      unifier.resolve_function(res_pair.as_referenced, res_pair.as_defined, tok_ptr);
    }

    move_from(resolution_instance.errors, type_errors);

    run_unify(unifier, substitution, external_functions, type_errors, unify_time);
    external_it = external_functions.visited_candidates.begin();

    if (arguments.show_ast) {
      StringVisitor str_visitor(&str_registry, &store);
      str_visitor.colorize = arguments.rich_text;
      std::cout << root_res->root_block->accept(str_visitor) << std::endl;
    }
  }

  const auto& local_func_types = library.get_local_function_types();
  ConcreteFunctionTypeInstance concrete_instance(library, store, str_registry);

  for (const auto& func_it : local_func_types) {
    if (can_show_untyped_function_errors(store, ast_store, func_it.first)) {
      //  Given that we successfully parsed the file and generated type constraints for it,
      //  ensure its type has no free type variables.
      check_for_concrete_function_type(concrete_instance, func_it.first, func_it.second);
    }
  }

  move_from(concrete_instance.errors, type_errors);

  const auto full_t1 = std::chrono::high_resolution_clock::now();
  const auto full_elapsed_ms = std::chrono::duration<double>(full_t1 - full_t0).count() * 1e3;
  const auto check_elapsed_ms = std::chrono::duration<double>(full_t1 - check_t0).count() * 1e3;
  const auto build_search_path_elapsed_ms =
    std::chrono::duration<double>(path_t1 - full_t0).count() * 1e3;

  if (arguments.show_type_distribution) {
    constraint_generator.show_type_distribution();
  }

  if (arguments.show_local_variable_types) {
    constraint_generator.show_variable_types(type_to_string);
  }

  if (arguments.show_local_function_types) {
    constraint_generator.show_local_function_types(type_to_string);
  }

  if (arguments.show_visited_external_files) {
    for (const auto& it : ast_store.asts) {
      std::cout << "Visited: " << it.first << std::endl;
    }
  }

  if (arguments.show_errors) {
    show_parse_errors(parse_errors, source_data_by_token, arguments);
    show_type_errors(type_errors, source_data_by_token, type_to_string, arguments);
  }

  if (arguments.show_diagnostics) {
    std::cout << "Num files visited: " << ast_store.num_visited_files() << std::endl;
    std::cout << "Num type eqs: " << substitution.num_type_equations() << std::endl;
    std::cout << "Subs size: " << substitution.num_bound_terms() << std::endl;
    std::cout << "Num types: " << type_store.size() << std::endl;
    std::cout << "Num external functions: " << external_functions.resolved_candidates.size() << std::endl;
    std::cout << "Parse / check time: " << check_elapsed_ms << " (ms)" << std::endl;
    std::cout << "Build path time: " << build_search_path_elapsed_ms << " (ms)" << std::endl;
    std::cout << "Unify time: " << unify_time << " (ms)" << std::endl;
    std::cout << "Constraint gen time: " << constraint_time << " (ms)" << std::endl;
    std::cout << "Total time: " << full_elapsed_ms << " (ms)" << std::endl;
  }

  return 0;
}