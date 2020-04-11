#include "parse_pipeline.hpp"
#include "ast_store.hpp"
#include "command_line.hpp"
#include <fstream>

namespace mt {
namespace {

void parse_file(ParseInstance* instance, const std::vector<Token>& tokens, std::string_view contents) {
  AstGenerator ast_gen;

  ast_gen.parse(instance, tokens, contents);
  if (instance->had_error) {
    return;
  }

  IdentifierClassifier classifier(instance->string_registry, instance->store, instance->file_descriptor, contents);
  classifier.transform_root(instance->root_block);

  auto& errs = classifier.get_errors();
  if (!errs.empty()) {
    instance->had_error = true;
    instance->errors = std::move(errs);
    return;
  }

  auto& warnings = classifier.get_warnings();
  instance->warnings = std::move(warnings);
}

FileScanResult scan_file(const std::string& file_path) {
  using std::swap;

  std::ifstream ifs(file_path);
  if (!ifs) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_file_io);
  }

  auto contents = std::make_unique<std::string>((std::istreambuf_iterator<char>(ifs)),
                                                (std::istreambuf_iterator<char>()));

  if (!mt::utf8::is_valid(*contents)) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_non_utf8_source);
  }

  mt::Scanner scanner;
  auto scan_result = scanner.scan(*contents);

  if (!scan_result) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_scan_failure);
  }

  auto scan_info = std::move(scan_result.value);

  auto insert_res = insert_implicit_expr_delimiters(scan_info.tokens, *contents);
  if (insert_res) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_scan_failure);
  }

  FileScanResult success;
  swap(success.value.scan_info, scan_info);
  swap(success.value.file_contents, contents);
  success.value.file_descriptor = CodeFileDescriptor(FilePath(file_path));

  return success;
}

void show_parse_errors(const ParseErrors& errs, const TokenSourceMap& sources_by_token, const cmd::Arguments& args) {
  mt::ShowParseErrors show;
  show.is_rich_text = args.rich_text;
  show.show(errs, sources_by_token);
}

const FileScanSuccess* try_scan_file(const FilePath& file_path, ScanResultStore& scan_results) {
  auto tmp_scan_result = scan_file(file_path.str());
  if (!tmp_scan_result) {
    std::cout << "Failed to scan file: " << file_path << std::endl;
    return nullptr;
  }

  scan_results.push_back(std::make_unique<FileScanSuccess>());
  auto& scan_result_val = *scan_results.back();
  swap(scan_result_val, tmp_scan_result.value);

  return scan_results.back().get();
}

void store_scanned_tokens(const FileScanSuccess& scan_result, TokenSourceMap& source_data_by_token) {
  auto& scan_info = scan_result.scan_info;
  const auto& tokens = scan_info.tokens;
  const auto contents = scan_result.file_contents.get();
  const auto file_descriptor = &scan_result.file_descriptor;

  for (const auto& tok : tokens) {
    TokenSourceMap::SourceData source_data(contents, file_descriptor, &scan_info.row_column_indices);
    source_data_by_token.insert(tok, source_data);
  }
}

const FileScanSuccess* run_scan_file(const FilePath& file_path,
                                     ScanResultStore& scan_results,
                                     TokenSourceMap& source_data_by_token) {
  const auto scan_result = try_scan_file(file_path, scan_results);
  if (scan_result) {
    store_scanned_tokens(*scan_result, source_data_by_token);
  }

  return scan_result;
}

AstStore::Entry* run_parse_file(ParseInstance& parse_instance,
                                const FileScanSuccess& scan_result,
                                AstStore& ast_store,
                                const TokenSourceMap& sources_by_token,
                                const cmd::Arguments& arguments) {

  const auto& scan_info = scan_result.scan_info;
  const auto& file_path = scan_result.file_descriptor.file_path;

  parse_file(&parse_instance, scan_info.tokens, *scan_result.file_contents);

  if (parse_instance.had_error) {
    if (arguments.show_errors) {
      show_parse_errors(parse_instance.errors, sources_by_token, arguments);
    }
    std::cout << "Failed to parse file: " << file_path << std::endl;
    ast_store.emplace_parse_failure(file_path);
    return nullptr;

  } else if (!parse_instance.warnings.empty() && arguments.show_warnings) {
    show_parse_errors(parse_instance.warnings, sources_by_token, arguments);
  }

  assert(parse_instance.root_block->type_scope->is_root());

  auto& root_block = parse_instance.root_block;
  return ast_store.insert(file_path, AstStore::Entry(std::move(root_block), true, false));
}

}

void swap(FileScanSuccess& a, FileScanSuccess& b) {
  using std::swap;

  swap(a.file_contents, b.file_contents);
  swap(a.file_descriptor, b.file_descriptor);
  swap(a.scan_info, b.scan_info);
}

ParsePipelineInstanceData::ParsePipelineInstanceData(const SearchPath& search_path, Store& store, TypeStore& type_store,
                                                     StringRegistry& str_registry, AstStore& ast_store,
                                                     ScanResultStore& scan_results, TokenSourceMap& source_data_by_token,
                                                     const cmd::Arguments& arguments) :
  search_path(search_path), store(store), type_store(type_store), string_registry(str_registry), ast_store(ast_store),
  scan_results(scan_results), source_data_by_token(source_data_by_token), arguments(arguments) {
  //
}

AstStore::Entry* file_entry(ParsePipelineInstanceData& pipe_instance, const FilePath& file_path) {
  const auto maybe_ast_entry = pipe_instance.ast_store.lookup(file_path);
  if (maybe_ast_entry) {
    return maybe_ast_entry;
  }

  const auto* scan_result = run_scan_file(file_path, pipe_instance.scan_results, pipe_instance.source_data_by_token);
  if (!scan_result) {
    pipe_instance.ast_store.emplace_parse_failure(file_path);
    return nullptr;
  }

  ParseInstance parse_instance(&pipe_instance.store, &pipe_instance.type_store, &pipe_instance.string_registry,
    &scan_result->file_descriptor, scan_result->scan_info.functions_are_end_terminated);

  const auto root_res = run_parse_file(parse_instance, *scan_result, pipe_instance.ast_store,
                                       pipe_instance.source_data_by_token, pipe_instance.arguments);
  if (!root_res) {
    return nullptr;
  }

  pipe_instance.roots.insert(root_res->root_block.get());

  for (auto& pending_import : parse_instance.pending_type_imports) {
    const auto& ident_str = pipe_instance.string_registry.at(pending_import.identifier);
    const auto search_res = pipe_instance.search_path.search_for(ident_str, fs::directory_name(file_path));
    if (!search_res) {
      std::cout << "Failed to resolve type import: " << ident_str << std::endl;
      return nullptr;
    }

    auto maybe_import = file_entry(pipe_instance, search_res.value()->defining_file);

    if (maybe_import && maybe_import->root_block) {
      auto exported_type_scope = maybe_import->root_block->type_scope;
      pending_import.into_scope->add_import(TypeImport(exported_type_scope, pending_import.is_exported));
    }
  }

  return root_res;
}

}
