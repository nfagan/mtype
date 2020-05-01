#include "app.hpp"
#include "show.hpp"
#include "type_analysis.hpp"

namespace mt {

namespace {
  template <typename T>
  void move_from(std::vector<T>& source, std::vector<T>& dest) {
    std::move(source.begin(), source.end(), std::back_inserter(dest));
  }

  bool are_valid_root_entries(const AstStoreEntries& entries) {
    for (const auto& entry : entries) {
      assert(entry);
      if (!entry->root_block || !entry->parsed_successfully) {
        return false;
      }
    }
    return true;
  }
}

App::App(const cmd::Arguments& args,
         SearchPath&& search_path_) :
  arguments(args),
  search_path(std::move(search_path_)),
  type_store(args.initial_store_capacity),
  library(type_store, store, search_path, string_registry),
  unifier(type_store, library, string_registry),
  constraint_generator(substitution, store, type_store, library, string_registry),
  type_to_string(&library, &string_registry) {
  //
  initialize();
}

void App::initialize() {
  configure_type_to_string(type_to_string, arguments);
  make_pre_imports();
  maybe_make_error_filter();
}

void App::maybe_make_error_filter() {
  bool require_error_filter = !arguments.error_filter_identifiers.empty();
  if (require_error_filter) {
    maybe_error_filter = ErrorFilter();
  }

  for (const auto& ident : arguments.error_filter_identifiers) {
    maybe_error_filter.value().identifiers.insert(ident);
  }
}

void App::add_root_identifier(const std::string& name,
                              const SearchCandidate* source_candidate) {
  MatlabIdentifier ident(string_registry.register_string(name));
  FunctionSearchCandidate search_candidate(source_candidate, ident);
  external_functions.add_visited_candidate(search_candidate);
}

void App::make_pre_imports() {
  for (const auto& pre_import_str : arguments.pre_imports) {
    pre_imports.emplace_back(pre_import_str);
    const auto& pre_import = pre_imports.back();

    const auto parse_source_data = pre_import.to_parse_source_data();
    const auto source_token = pre_import.make_token();

    source_data_by_token.insert(source_token, parse_source_data);
  }
}

bool App::locate_root_identifiers() {
  for (const auto& name : arguments.root_identifiers) {
    const auto maybe_source_candidate = search_path.search_for(name);
    if (!maybe_source_candidate) {
      std::cout << "No file on the search path matched: " << name << std::endl;
      return false;
    }

    add_root_identifier(name, maybe_source_candidate.value());
  }

  return true;
}

void App::check_for_concrete_function_types() {
  const auto& local_func_types = library.get_local_function_types();
  ConcreteFunctionTypeInstance concrete_instance(library, store, string_registry);

  for (const auto& func_it : local_func_types) {
    if (can_show_untyped_function_errors(store, ast_store, func_it.first)) {
      //  Given that we successfully parsed the file and generated type constraints for it,
      //  ensure its type has no free type variables.
      check_for_concrete_function_type(concrete_instance, func_it.first, func_it.second);
    }
  }

  move_from(concrete_instance.errors, type_errors);
}

void App::unify() {
  auto unify_res = unifier.unify(&substitution, &external_functions);

  if (unify_res.is_error()) {
    move_from(unify_res.errors, type_errors);
    if (unify_res.errors.empty()) {
      std::cout << "ERROR: An error occurred, but no explicit errors were generated." << std::endl;
    }
  }
}

bool App::add_base_scopes(const AstStoreEntries& entries) const {
  for (const auto& entry : entries) {
    if (!entry->added_base_type_scope) {
      entry->root_block->type_scope->add_import(TypeImport(library.base_scope, false));
      entry->added_base_type_scope = true;
    }
  }

  return true;
}

bool App::resolve_type_imports(AstStoreEntryPtr root_entry) {
  if (root_entry->resolved_type_imports) {
    return true;
  }

  TypeImportResolutionInstance import_res(source_data_by_token);
  auto resolution_result =
    maybe_resolve_type_imports(root_entry->root_block->type_scope, import_res);
  root_entry->resolved_type_imports = true;

  if (!resolution_result.success) {
    move_from(import_res.errors, parse_errors);
  }

  return resolution_result.success;
}

namespace {
  void clear_destructured_tuple(TypeStore& store, types::DestructuredTuple& tuple) {
    for (auto& member : tuple.members) {
      if (!member->is_variable()) {
        member = store.make_fresh_type_variable_reference();
      }
    }
  }

  void maybe_clear_destructured_tuple(TypeStore& store, Type* type) {
    if (type->is_destructured_tuple()) {
      clear_destructured_tuple(store, MT_DT_MUT_REF(*type));
    }
  }

  void clear_abstraction(TypeStore& store, Type* current_type) {
    auto current_source = current_type->scheme_source();
    if (current_source->is_abstraction()) {
      auto& abstr = MT_ABSTR_MUT_REF(*current_source);
      maybe_clear_destructured_tuple(store, abstr.inputs);
      maybe_clear_destructured_tuple(store, abstr.outputs);
    }
  }
}

void App::clear_function_types(const CodeFileDescriptor* for_file) {
  auto maybe_functions = functions_by_file.lookup(for_file);
  if (!maybe_functions) {
    return;
  }

  const auto& funcs = *maybe_functions.value();
  for (const auto& func : funcs) {
    auto maybe_current_type = library.lookup_local_function(func);
    if (maybe_current_type) {
      clear_abstraction(type_store, maybe_current_type.value());
    }
  }
}

void App::clear_function_types(const AstStoreEntries& root_entries) {
  for (const auto& entry : root_entries) {
    const auto* file_descriptor = entry->root_block->scope->file_descriptor;
    clear_function_types(file_descriptor);
  }
}

bool App::check_for_recursive_types(const AstStoreEntries& root_entries,
                                    const PendingSchemes& pending_schemes) {
  bool success = true;

  for (const auto& root_entry : root_entries) {
    CheckRecursiveTypesInstance recursive_types_instance(&pending_schemes);
    const auto& type_scope = root_entry->root_block->type_scope;
    mt::check_for_recursive_types(recursive_types_instance, type_scope);

    if (recursive_types_instance.had_error) {
      success = false;
      move_from(recursive_types_instance.errors, type_errors);
    }
  }

  for (const auto& pending_scheme : pending_schemes) {
    CheckRecursiveTypesInstance recursive_types_instance(&pending_schemes);
    const auto scheme = pending_scheme.scheme;
    const auto& source_token = pending_scheme.source_token;
    check_for_recursive_type(recursive_types_instance, scheme, source_token);

    if (recursive_types_instance.had_error) {
      success = false;
      move_from(recursive_types_instance.errors, type_errors);
    }
  }

  return success;
}

bool App::resolve_type_identifiers(const AstStoreEntries& root_entries) {
  bool any_resolution_errors = false;
  PendingSchemes pending_schemes;

  for (const auto& root_entry : root_entries) {
    if (root_entry->resolved_type_identifiers) {
      continue;
    }

    TypeIdentifierResolverInstance instance(type_store, library, store,
                                            string_registry, source_data_by_token);
    TypeIdentifierResolver type_identifier_resolver(&instance);

    const auto& root = root_entry->root_block;
    root->accept(type_identifier_resolver);
    root_entry->resolved_type_identifiers = true;

    if (instance.had_error()) {
      move_from(instance.errors, parse_errors);
      any_resolution_errors = true;
    } else {
      //  Push schemes that need instantiation.
      move_from(instance.pending_schemes, pending_schemes);
    }
  }

  if (any_resolution_errors ||
      !check_for_recursive_types(root_entries, pending_schemes)) {
    clear_function_types(root_entries);
    return false;
  }

  for (const auto& pending_scheme : pending_schemes) {
    pending_scheme.instantiate(type_store);
  }

  return true;
}

bool App::resolve_external_functions(ParsePipelineInstanceData& pipeline_instance) {
  ResolutionInstance resolution_instance;
  auto resolution_pairs =
    mt::resolve_external_functions(resolution_instance, pipeline_instance,
                                   external_functions, visited_resolution_pairs);

  for (const auto& res_pair : resolution_pairs) {
    const auto source_token = res_pair.as_referenced_source_token;
    unifier.resolve_function(res_pair.as_referenced, res_pair.as_defined, source_token);
  }

  move_from(resolution_instance.errors, type_errors);
  return true;
}

bool App::generate_type_constraints(const AstStoreEntries& root_entries) {
  for (const auto& root_entry : root_entries) {
    if (!root_entry->generated_type_constraints) {
      root_entry->root_block->accept_const(constraint_generator);
      root_entry->generated_type_constraints = true;
    }
  }

  auto& gen_warnings = constraint_generator.get_warnings();
  move_from(gen_warnings, type_errors);
  gen_warnings.clear();

  return true;
}

bool App::unify_while_able(ParsePipelineInstanceData& pipeline_instance) {
  bool proceed = true;
  while (proceed) {
    //  By unifying, we may discover the types of dependent external functions.
    //  By resolving these types, we may be able to locate additional dependent
    //  external functions. Continue to unify until we find no new external function types.
    const auto original_num_pending =
      external_functions.num_pending_candidate_files();

    if (!resolve_external_functions(pipeline_instance)) {
      return false;
    }

    unify();

    const auto new_num_pending = external_functions.num_pending_candidate_files();
    proceed = new_num_pending != original_num_pending;
  }

  return true;
}

bool App::visit_file(const FilePath& file_path) {
  if (ast_store.lookup(file_path)) {
    return false;
  }

  ParsePipelineInstanceData pipeline_instance(search_path, store, type_store, library,
                                              string_registry, ast_store,
                                              scan_result_store, functions_by_file,
                                              pre_imports, source_data_by_token, arguments,
                                              parse_errors, parse_warnings);

  auto root_res = file_entry(pipeline_instance, file_path);
  if (!root_res) {
    return false;
  }

  auto root_entries = pipeline_instance.gather_root_entries();
  if (!are_valid_root_entries(root_entries)) {
    return false;
  }

  if (!add_base_scopes(root_entries)) {
    return false;
  }

  if (!resolve_type_imports(root_res)) {
    return false;
  }

  if (!resolve_type_identifiers(root_entries)) {
    return false;
  }

  if (!generate_type_constraints(root_entries)) {
    return false;
  }

  if (!unify_while_able(pipeline_instance)) {
    return false;
  }

  return true;
}

void App::maybe_show() const {
  maybe_show_type_distribution();
  maybe_show_local_variable_types();
  maybe_show_local_function_types();
  maybe_show_visited_external_files();
  maybe_show_errors();
  maybe_show_diagnostics();
  maybe_show_asts();
}

void App::maybe_show_local_function_types() const {
  if (arguments.show_local_function_types) {
    show_function_types(functions_by_file, type_to_string,
                        store, string_registry, library);
  }
}

void App::maybe_show_local_variable_types() const {
  if (arguments.show_local_variable_types) {
    constraint_generator.show_variable_types(type_to_string);
  }
}

void App::maybe_show_visited_external_files() const {
  if (arguments.show_visited_external_files) {
    for (const auto& it : ast_store.asts) {
      std::cout << "Visited: " << it.first << std::endl;
    }
  }
}

namespace {
  template <typename T>
  bool passes_error_filter(const T& arg,
                           const ErrorFilter& filter,
                           const TokenSourceMap& source_data) {
    const auto source_token = arg->get_source_token();
    const auto maybe_source_data = source_data.lookup(source_token);
    if (maybe_source_data) {
      const auto& file_path = maybe_source_data.value().file_descriptor->file_path;
      return filter.matches_identifier(file_path);
    } else {
      return false;
    }
  }
}

std::vector<const TypeError*> App::filter_type_errors() const {
  std::vector<const TypeError*> filtered_errs;
  for (const auto& err : type_errors) {
    if (passes_error_filter(err, maybe_error_filter.value(), source_data_by_token)) {
      filtered_errs.push_back(err.get());
    }
  }
  return filtered_errs;
}

void App::show_type_errors() const {
  if (maybe_error_filter) {
    const auto filtered_errs = filter_type_errors();
    mt::show_type_errors(filtered_errs, source_data_by_token, type_to_string);

  } else {
    mt::show_type_errors(type_errors, source_data_by_token, type_to_string);
  }
}

void App::maybe_show_errors() const {
  if (arguments.show_errors) {
    show_parse_errors(parse_errors, source_data_by_token, arguments);
    show_type_errors();
  }
}

void App::maybe_show_diagnostics() const {
  if (arguments.show_diagnostics) {
    std::cout << "Num files visited: " << ast_store.num_visited_files() << std::endl;
    std::cout << "Num type eqs: " << substitution.num_type_equations() << std::endl;
    std::cout << "Subs size: " << substitution.num_bound_terms() << std::endl;
    std::cout << "Num types: " << type_store.size() << std::endl;
    std::cout << "Num external functions: "
              << external_functions.resolved_candidates.size() << std::endl;
//    std::cout << "Parse / check time: " << check_elapsed_ms << " (ms)" << std::endl;
//    std::cout << "Build path time: " << build_search_path_elapsed_ms << " (ms)" << std::endl;
//    std::cout << "Unify time: " << unify_time << " (ms)" << std::endl;
//    std::cout << "Constraint gen time: " << constraint_time << " (ms)" << std::endl;
//    std::cout << "Total time: " << full_elapsed_ms << " (ms)" << std::endl;
  }
}

void App::maybe_show_type_distribution() const {
  if (arguments.show_type_distribution) {
    auto counts = type_store.type_distribution();
    const auto num_types = double(type_store.size());

    for (const auto& ct : counts) {
      std::cout << to_string(ct.first) << ": " << ct.second << " (";
      std::cout << ct.second/num_types << ")" << std::endl;
    }
  }
}

void App::maybe_show_asts() const {
  if (arguments.show_ast) {
    show_asts(ast_store, store, string_registry, arguments);
  }
}

}
