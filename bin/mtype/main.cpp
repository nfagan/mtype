#include "mt/mt.hpp"
#include "command_line.hpp"
#include "ast_store.hpp"
#include "parse_pipeline.hpp"
#include "import_resolution.hpp"
#include <chrono>

namespace {

template <typename... Args>
mt::TypeToString make_type_to_string(Args&&... args) {
  mt::TypeToString type_to_string(std::forward<Args>(args)...);
  return type_to_string;
}

void configure_type_to_string(mt::TypeToString& type_to_string, const mt::cmd::Arguments& args) {
  type_to_string.explicit_destructured_tuples = false;
  type_to_string.arrow_function_notation = true;
  type_to_string.max_num_type_variables = 3;
  type_to_string.show_class_source_type = args.show_class_source_type;
  type_to_string.rich_text = args.rich_text;
}

bool locate_root_identifiers(const std::vector<std::string>& idents,
                             const mt::SearchPath& search_path,
                             mt::PendingExternalFunctions& external_functions,
                             mt::TypeStore& type_store) {
  for (const auto& ident : idents) {
    const auto maybe_source_candidate = search_path.search_for(ident);
    if (!maybe_source_candidate) {
      std::cout << "No file on the search path matched: " << ident << std::endl;
      return false;
    }

    const auto source_candidate = maybe_source_candidate.value();
    external_functions.add_candidate(source_candidate,
      external_functions.require_candidate_type(source_candidate, type_store));
  }

  return true;
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
    maybe_search_path = SearchPath::build_from_path_file(arguments.search_path_file_path);
  } else {
    maybe_search_path = SearchPath::build_from_paths(arguments.search_paths);
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

  //  Reserve space
  TypeStore type_store(arguments.initial_store_capacity);
  Library library(type_store, store, search_path, str_registry);
  library.make_known_types();

  auto type_to_string = make_type_to_string(&library, &str_registry);
  configure_type_to_string(type_to_string, arguments);

  Substitution substitution;
  Unifier unifier(type_store, library, str_registry);
  TypeConstraintGenerator constraint_generator(substitution, store, type_store, library, str_registry);

  PendingExternalFunctions external_functions;
  bool lookup_res = locate_root_identifiers(arguments.root_identifiers, search_path, external_functions, type_store);
  if (!lookup_res) {
    return -1;
  }

  AstStore ast_store;
  ScanResultStore scan_results;
  std::vector<std::unique_ptr<Token>> temporary_source_tokens;

  TokenSourceMap source_data_by_token;
  double constraint_time = 0;
  double unify_time = 0;

  auto external_it = external_functions.candidates.begin();
  while (external_it != external_functions.candidates.end()) {
    using clock = std::chrono::high_resolution_clock;

    const auto candidate_it = external_it++;
    const auto& candidate = candidate_it->first;
    const auto& file_path = candidate->defining_file;

    ParsePipelineInstanceData pipeline_instance(search_path, store, type_store, str_registry,
      ast_store, scan_results, source_data_by_token, arguments);

    auto root_res = file_entry(pipeline_instance, file_path);
    if (!root_res || !root_res->root_block || root_res->generated_type_constraints) {
      continue;
    }

    for (auto& root : pipeline_instance.roots) {
//      root->type_scope->add_import(TypeImport(library.base_scope, false));
    }

    TypeImportResolutionInstance import_res(source_data_by_token);
    auto resolution_result = maybe_resolve_type_imports(root_res->root_block->type_scope, import_res);
    if (!resolution_result.success) {
      ShowParseErrors show_errs;
      show_errs.show(import_res.errors, source_data_by_token);
      continue;
    }

    //  Type identifier resolution
    TypeIdentifierResolverInstance instance(type_store, store);
    TypeIdentifierResolver type_identifier_resolver(&instance);

    for (auto& root : pipeline_instance.roots) {
      root->accept_const(type_identifier_resolver);
    }

    for (const auto& unresolved : instance.unresolved_identifiers) {
      std::cout << "Unresolved: " << str_registry.at(unresolved.identifier.full_name()) << std::endl;
    }

    if (instance.had_error()) {
      ShowTypeErrors show(type_to_string);
      show.show(instance.errors, source_data_by_token);
    }

    const auto& root_ptr = root_res->root_block;
    const auto constraint_t0 = clock::now();
    root_ptr->accept_const(constraint_generator);
    root_res->generated_type_constraints = true;  //  Mark that constraints were generated.
    const auto constraint_t1 = clock::now();
    constraint_time += std::chrono::duration<double>(constraint_t1 - constraint_t0).count() * 1e3;

    Optional<Type*> maybe_local_func;
    Optional<FunctionDefNode*> maybe_top_level_func;
    Optional<FunctionReference> maybe_local_func_ref;

    auto code_file_type = code_file_type_from_root_block(*root_ptr);

    if (code_file_type == CodeFileType::function_def) {
      maybe_top_level_func = root_ptr->extract_top_level_function_def();

    } else if (code_file_type == CodeFileType::class_def) {
      maybe_top_level_func = root_ptr->extract_constructor_function_def(store);

    } else {
      std::cout << "Top level kind for: " << file_path << ": " << to_string(code_file_type) << std::endl;
    }

    if (maybe_top_level_func) {
      auto ref = store.get(maybe_top_level_func.value()->ref_handle);
      maybe_local_func = library.lookup_local_function(ref.def_handle);
      maybe_local_func_ref = Optional<FunctionReference>(ref);
    }

    if (maybe_local_func) {
      assert(maybe_local_func_ref);
      const auto& ref = maybe_local_func_ref.value();

      const auto func_name = str_registry.at(ref.name.full_name());
      const auto func_dir = fs::directory_name(ref.scope->file_descriptor->file_path);
      auto search_res = search_path.search_for(func_name, func_dir);
      Token source_token;

      store.use<Store::ReadConst>([&](const auto& reader) {
        source_token = reader.at(ref.def_handle).header.name_token;
      });

      temporary_source_tokens.push_back(std::make_unique<Token>(source_token));

      if (search_res && external_functions.has_candidate(search_res.value())) {
        const auto* use_token = temporary_source_tokens.back().get();
        const auto curr_type = external_functions.candidates.at(search_res.value());

        const auto lhs_term = make_term(use_token, maybe_local_func.value());
        const auto rhs_term = make_term(use_token, curr_type);
        substitution.push_type_equation(make_eq(lhs_term, rhs_term));
      }
    }

    const auto unify_t0 = clock::now();
    auto unify_res = unifier.unify(&substitution, &external_functions);
    external_it = external_functions.candidates.begin();
    const auto unify_t1 = clock::now();
    unify_time += std::chrono::duration<double>(unify_t1 - unify_t0).count() * 1e3;

    if (unify_res.is_error() && arguments.show_errors) {
      ShowTypeErrors show(type_to_string);
      show.show(unify_res.errors, source_data_by_token);

      if (unify_res.errors.empty()) {
        std::cout << "ERROR: An error occurred, but no explicit errors were generated." << std::endl;
      }
    }

    if (arguments.show_ast) {
      StringVisitor str_visitor(&str_registry, &store);
      str_visitor.colorize = arguments.rich_text;
      std::cout << root_ptr->accept(str_visitor) << std::endl;
    }
  }

  const auto full_t1 = std::chrono::high_resolution_clock::now();
  const auto full_elapsed_ms = std::chrono::duration<double>(full_t1 - full_t0).count() * 1e3;
  const auto check_elapsed_ms = std::chrono::duration<double>(full_t1 - check_t0).count() * 1e3;
  const auto build_search_path_elapsed_ms = std::chrono::duration<double>(path_t1 - full_t0).count() * 1e3;

  if (arguments.show_type_distribution) {
    constraint_generator.show_type_distribution();
  }

  if (arguments.show_local_variable_types) {
    constraint_generator.show_variable_types(type_to_string);
  }

  if (arguments.show_local_function_types) {
    constraint_generator.show_local_function_types(type_to_string);
  }

  if (arguments.show_diagnostics) {
    std::cout << "Num files visited: " << ast_store.num_visited_files() << std::endl;
    std::cout << "Num type eqs: " << substitution.num_type_equations() << std::endl;
    std::cout << "Subs size: " << substitution.num_bound_terms() << std::endl;
    std::cout << "Num types: " << type_store.size() << std::endl;
    std::cout << "Num external functions: " << external_functions.candidates.size() << std::endl;
    std::cout << "Parse / check time: " << check_elapsed_ms << " (ms)" << std::endl;
    std::cout << "Build path time: " << build_search_path_elapsed_ms << " (ms)" << std::endl;
    std::cout << "Unify time: " << unify_time << " (ms)" << std::endl;
    std::cout << "Constraint gen time: " << constraint_time << " (ms)" << std::endl;
    std::cout << "Total time: " << full_elapsed_ms << " (ms)" << std::endl;
  }

  return 0;
}