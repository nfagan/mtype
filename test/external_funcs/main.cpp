#include "mt/mt.hpp"
#include "util.hpp"
#include "command_line.hpp"
#include <chrono>

namespace {

void show_parse_errors(const mt::ParseErrors& errs, const mt::TextRowColumnIndices& inds) {
  mt::ShowParseErrors show(&inds);
  show.is_rich_text = true;
  show.show(errs);
}

template <typename... Args>
mt::TypeToString make_type_to_string(Args&&... args) {
  mt::TypeToString type_to_string(std::forward<Args>(args)...);
  type_to_string.explicit_destructured_tuples = false;
  type_to_string.arrow_function_notation = true;
  type_to_string.max_num_type_variables = 3;
  return type_to_string;
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

  if (arguments.had_unrecognized_argument) {
    return 0;
  }

  const auto maybe_search_path = SearchPath::build_from_path_file(arguments.search_path_file_path);
  if (!maybe_search_path) {
    std::cout << "Failed to build search path." << std::endl;
    return -1;
  }

  if (arguments.root_identifiers.empty()) {
    std::cout << "Specify at least one entry function." << std::endl;
    return 0;
  }

  const auto path_t1 = std::chrono::high_resolution_clock::now();
  //  Time excluding time to build the search path
  const auto check_t0 = std::chrono::high_resolution_clock::now();

  const auto& search_path = maybe_search_path.value();

  Store store;
  StringRegistry str_registry;

  //  Reserve space
  TypeStore type_store(1e5);
  Library library(type_store, store, search_path, str_registry);
  library.make_known_types();

  const auto type_to_string = make_type_to_string(&library, &str_registry);

  Substitution substitution;
  Unifier unifier(type_store, library, str_registry);
  TypeConstraintGenerator constraint_generator(substitution, store, type_store, library, str_registry);

  PendingExternalFunctions external_functions;
  bool lookup_res = locate_root_identifiers(arguments.root_identifiers, search_path, external_functions, type_store);
  if (!lookup_res) {
    return -1;
  }

  std::unordered_set<FilePath, FilePath::Hash> visited_external_functions;
  auto external_it = external_functions.candidates.begin();

  std::vector<BoxedRootBlock> root_blocks;
  std::vector<std::unique_ptr<FileScanSuccess>> scan_results;
  std::vector<std::unique_ptr<Token>> temporary_source_tokens;

  TokenSourceMap source_data_by_token;

  while (external_it != external_functions.candidates.end()) {
    const auto candidate_it = external_it++;
    const auto& candidate = candidate_it->first;
    const auto& file_path = candidate->defining_file;

    if (visited_external_functions.count(file_path) > 0) {
      continue;
    } else {
      visited_external_functions.insert(file_path);
    }

    auto tmp_scan_result = scan_file(file_path.str());
    if (!tmp_scan_result) {
      std::cout << "Failed to scan file: " << file_path << std::endl;
      return -1;
    }

    scan_results.push_back(std::make_unique<FileScanSuccess>());
    auto& scan_result_val = *scan_results.back();
    swap(scan_result_val, tmp_scan_result.value);
    const auto& scan_result = *scan_results.back();

    auto& scan_info = scan_result.scan_info;
    const auto& tokens = scan_info.tokens;
    const auto& contents = scan_result.file_contents;
    const auto& file_descriptor = scan_result.file_descriptor;
    const auto end_terminated = scan_info.functions_are_end_terminated;

    for (const auto& tok : tokens) {
      TokenSourceMap::SourceData source_data(&contents, &file_descriptor, &scan_info.row_column_indices);
      source_data_by_token.insert(tok, source_data);
    }

    AstGenerator::ParseInputs parse_inputs(&str_registry, &store, &file_descriptor, end_terminated);
    auto parse_result = parse_file(tokens, contents, parse_inputs);

    if (!parse_result) {
      show_parse_errors(parse_result.error.errors, scan_info.row_column_indices);
      std::cout << "Failed to parse file: " << file_path << std::endl;
      return -1;

    } else if (!parse_result.value.warnings.empty()) {
      show_parse_errors(parse_result.value.warnings, scan_info.row_column_indices);
    }

    auto root_block = std::move(parse_result.value.root_block);
    const auto* root_ptr = root_block.get();
    root_blocks.emplace_back(std::move(root_block));

    root_ptr->accept_const(constraint_generator);

    auto code_file_type = code_file_type_from_root_block(*root_ptr);
    if (code_file_type == CodeFileType::function_def) {
      auto maybe_top_level_func = root_ptr->top_level_function_def();
      if (maybe_top_level_func) {
        auto ref = store.get(maybe_top_level_func.value()->ref_handle);
        auto maybe_local_func = library.lookup_local_function(ref.def_handle);
        if (maybe_local_func) {
          auto func_name = str_registry.at(ref.name.full_name());
          auto search_res = search_path.search_for(func_name,
            fs::directory_name(ref.scope->file_descriptor->file_path));
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
      }
    } else {
      std::cout << "Top level kind for: " << file_path << ": " << to_string(code_file_type) << std::endl;
    }

    auto unify_res = unifier.unify(&substitution, &external_functions);
    external_it = external_functions.candidates.begin();

    if (unify_res.is_error()) {
      ShowUnificationErrors show(type_to_string);
      show.show(unify_res.errors, source_data_by_token);

      if (unify_res.errors.empty()) {
        std::cout << "ERROR: An error occurred, but no explicit errors were generated." << std::endl;
      }
    }

    if (arguments.show_ast) {
      StringVisitor str_visitor(&str_registry, &store);
      std::cout << root_ptr->accept(str_visitor) << std::endl;
    }
  }

  const auto full_t1 = std::chrono::high_resolution_clock::now();
  const auto full_elapsed_ms = std::chrono::duration<double>(full_t1 - full_t0).count() * 1e3;
  const auto check_elapsed_ms = std::chrono::duration<double>(full_t1 - check_t0).count() * 1e3;
  const auto build_search_path_elapsed_ms = std::chrono::duration<double>(path_t1 - full_t0).count() * 1e3;

  if (arguments.show_local_variable_types) {
    constraint_generator.show_variable_types(type_to_string);
  }

  if (arguments.show_local_function_types) {
    constraint_generator.show_local_function_types(type_to_string);
  }

  if (arguments.show_diagnostics) {
    std::cout << "Num type eqs: " << substitution.num_type_equations() << std::endl;
    std::cout << "Subs size: " << substitution.num_bound_terms() << std::endl;
    std::cout << "Num types: " << type_store.size() << std::endl;
    std::cout << "Num external functions: " << external_functions.candidates.size() << std::endl;
    std::cout << "Parse / check time: " << check_elapsed_ms << " (ms)" << std::endl;
    std::cout << "Build path time: " << build_search_path_elapsed_ms << " (ms)" << std::endl;
    std::cout << "Total time: " << full_elapsed_ms << " (ms)" << std::endl;
  }

  return 0;
}