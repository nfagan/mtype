#include "parse_pipeline.hpp"
#include "ast_store.hpp"
#include "command_line.hpp"
#include <fstream>

namespace mt {
namespace {

void parse_file(ParseInstance* instance, const std::vector<Token>& tokens) {
  AstGenerator ast_gen;

  ast_gen.parse(instance, tokens);
  if (instance->had_error) {
    return;
  }

  IdentifierClassifier classifier(instance->string_registry, instance->store, instance->source_data);
  classifier.transform_root(instance->root_block);

  auto& errs = classifier.get_errors();
  if (!errs.empty()) {
    instance->had_error = true;
    instance->errors = std::move(errs);
  }

  auto& warnings = classifier.get_warnings();
  instance->warnings = std::move(warnings);
}

FileScanResult scan_file(const FilePath& file_path) {
  using std::swap;
  auto maybe_contents = fs::read_file(file_path);
  if (!maybe_contents) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_file_io);
  }

  auto contents = std::move(maybe_contents.rvalue());
  if (!utf8::is_valid(*contents)) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_non_utf8_source);
  }

  Scanner scanner;
  auto scan_result = scanner.scan(*contents);
  if (!scan_result) {
    return make_error<FileScanError, FileScanSuccess>(std::move(scan_result.error));
  }

  auto scan_info = std::move(scan_result.value);

  auto maybe_insert_err = insert_implicit_expr_delimiters(scan_info.tokens, *contents);
  if (maybe_insert_err) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_scan_failure);
  }

  FileScanResult success;
  swap(success.value.scan_info, scan_info);
  swap(success.value.file_contents, contents);
  success.value.file_descriptor = CodeFileDescriptor(FilePath(file_path));

  return success;
}

const FileScanSuccess* try_scan_file(const FilePath& file_path, ScanResultStore& scan_results) {
  auto tmp_scan_result = scan_file(file_path);
  if (!tmp_scan_result) {
    std::cout << "Failed to scan file: " << file_path << std::endl;
    return nullptr;
  }

  scan_results[file_path] = std::make_unique<FileScanSuccess>();
  auto& scan_result_val = *scan_results.at(file_path);
  swap(scan_result_val, tmp_scan_result.value);

  return scan_results.at(file_path).get();
}

void store_scanned_tokens(const ParseSourceData& source_data,
                          const std::vector<Token>& tokens,
                          TokenSourceMap& source_data_by_token) {
  for (const auto& tok : tokens) {
    source_data_by_token.insert(tok, source_data);
  }
}

AstStore::Entry* run_parse_file(ParseInstance& parse_instance,
                                const FileScanSuccess& scan_result,
                                ParsePipelineInstanceData& pipeline_instance) {
  const auto& scan_info = scan_result.scan_info;
  const auto& file_path = scan_result.file_descriptor.file_path;
  auto& ast_store = pipeline_instance.ast_store;
  auto& sources_by_token = pipeline_instance.source_data_by_token;
  const auto& args = pipeline_instance.arguments;

  parse_file(&parse_instance, scan_info.tokens);

  if (parse_instance.had_error) {
    pipeline_instance.add_errors(parse_instance.errors);
    ast_store.emplace_parse_failure(file_path);
    return nullptr;
  } else if (!parse_instance.warnings.empty()) {
    pipeline_instance.add_warnings(parse_instance.warnings);
  }

  assert(parse_instance.root_block->type_scope->is_root());
  auto& root_block = parse_instance.root_block;
  const auto& maybe_class_def = parse_instance.file_entry_class_def;
  const auto& maybe_function_def = parse_instance.file_entry_function_ref;

  AstStore::Entry entry(std::move(root_block), maybe_class_def, maybe_function_def);
  return ast_store.insert(file_path, std::move(entry));
}

ParseError make_error_unresolved_file(const ParseSourceData& source_data,
                                      const Token& token,
                                      const std::string& ident) {
  std::string msg = "Failed to locate: `" + ident + "`.";
  return ParseError(source_data.source, token, msg, source_data.file_descriptor);
}

ParseError make_error_superclass_is_not_class_file(const ParseSourceData& source_data,
                                                   const Token& token,
                                                   const std::string& ident) {
  std::string msg = "Superclass `" + ident + "` is not a classdef file.";
  return ParseError(source_data.source, token, msg, source_data.file_descriptor);
}

bool traverse_imports(const PendingTypeImports& pending_type_imports,
                      ParsePipelineInstanceData& pipe_instance,
                      const ParseSourceData& source_data) {
  bool success = true;
  const auto search_dir = fs::directory_name(source_data.file_descriptor->file_path);

  for (auto& pending_import : pending_type_imports) {
    const auto ident_str = pipe_instance.string_registry.at(pending_import.identifier);
    const auto search_res = pipe_instance.search_path.search_for(ident_str, search_dir);
    if (!search_res) {
      auto err = make_error_unresolved_file(source_data, pending_import.source_token, ident_str);
      pipe_instance.add_error(err);
      success = false;
      continue;
    }

    auto maybe_import = file_entry(pipe_instance, search_res.value()->defining_file);
    if (!maybe_import || !maybe_import->root_block) {
      success = false;
      continue;
    }

    auto exported_type_scope = maybe_import->root_block->type_scope;
    pending_import.into_scope->add_import(TypeImport(exported_type_scope, pending_import.is_exported));
  }

  return success;
}

bool traverse_superclasses(const ClassDefHandle& class_def,
                           ParsePipelineInstanceData& pipe_instance,
                           const ParseSourceData& source_data) {
  std::vector<ClassDef::Superclass> superclasses;
  Token source_token;

  pipe_instance.store.use<Store::ReadConst>([&](const auto& reader) {
    const ClassDef& def = reader.at(class_def);
    superclasses = def.superclasses;
    source_token = def.source_token;
  });

  bool success = true;
  const auto search_dir = fs::directory_name(source_data.file_descriptor->file_path);

  for (int64_t i = 0; i < int64_t(superclasses.size()); i++) {
    const auto& superclass = superclasses[i];
    const auto ident_str = pipe_instance.string_registry.at(superclass.name.full_name());
    const auto search_res = pipe_instance.search_path.search_for(ident_str, search_dir);
    if (!search_res) {
      auto err = make_error_unresolved_file(source_data, source_token, ident_str);
      pipe_instance.add_error(err);
      success = false;
      continue;
    }

    auto maybe_superclass = file_entry(pipe_instance, search_res.value()->defining_file);
    if (!maybe_superclass || !maybe_superclass->root_block) {
      success = false;
      continue;
    }

    const auto& maybe_class_def = maybe_superclass->file_entry_class_def;
    if (!maybe_class_def.is_valid()) {
      auto err = make_error_superclass_is_not_class_file(source_data, source_token, ident_str);
      pipe_instance.add_error(err);
      success = false;
      continue;
    }

    //  Register the super class definition.
    pipe_instance.store.use<Store::ReadMut>([&](auto& reader) {
      ClassDef& def = reader.at(class_def);
      auto& super_ref = def.superclasses[i];
      assert(!super_ref.def_handle.is_valid());
      super_ref.def_handle = maybe_class_def;
    });
  }

  return success;
}

}

void swap(FileScanSuccess& a, FileScanSuccess& b) {
  using std::swap;

  swap(a.file_contents, b.file_contents);
  swap(a.file_descriptor, b.file_descriptor);
  swap(a.scan_info, b.scan_info);
}

ParsePipelineInstanceData::ParsePipelineInstanceData(const SearchPath& search_path,
                                                     Store& store,
                                                     TypeStore& type_store,
                                                     Library& library,
                                                     StringRegistry& str_registry,
                                                     AstStore& ast_store,
                                                     ScanResultStore& scan_results,
                                                     TokenSourceMap& source_data_by_token,
                                                     const cmd::Arguments& arguments,
                                                     ParseErrors& parse_errors,
                                                     ParseErrors& parse_warnings) :
  search_path(search_path),
  store(store),
  type_store(type_store),
  library(library),
  string_registry(str_registry),
  ast_store(ast_store),
  scan_results(scan_results),
  source_data_by_token(source_data_by_token),
  arguments(arguments),
  parse_errors(parse_errors),
  parse_warnings(parse_warnings) {
  //
}

void ParsePipelineInstanceData::add_error(const ParseError& err) {
  parse_errors.push_back(err);
}

void ParsePipelineInstanceData::add_errors(const ParseErrors& errs) {
  parse_errors.insert(parse_errors.end(), errs.cbegin(), errs.cend());
}

void ParsePipelineInstanceData::add_warnings(const ParseErrors& warnings) {
  parse_warnings.insert(parse_warnings.end(), warnings.cbegin(), warnings.cend());
}

AstStore::Entry* file_entry(ParsePipelineInstanceData& pipe_instance, const FilePath& file_path) {
  const auto maybe_ast_entry = pipe_instance.ast_store.lookup(file_path);
  if (maybe_ast_entry) {
    return maybe_ast_entry;
  }

  const auto scan_result = try_scan_file(file_path, pipe_instance.scan_results);
  ParseSourceData source_data;

  if (scan_result) {
    source_data = scan_result->to_parse_source_data();
    const auto& tokens = scan_result->scan_info.tokens;
    store_scanned_tokens(source_data, tokens, pipe_instance.source_data_by_token);
  } else {
    pipe_instance.ast_store.emplace_parse_failure(file_path);
    return nullptr;
  }

  ParseInstance parse_instance(&pipe_instance.store, &pipe_instance.type_store, &pipe_instance.library,
    &pipe_instance.string_registry, source_data, scan_result->scan_info.functions_are_end_terminated);

  const auto root_res = run_parse_file(parse_instance, *scan_result, pipe_instance);
  if (!root_res) {
    return nullptr;
  }

  pipe_instance.roots.insert(root_res->root_block.get());
  pipe_instance.root_files.insert(file_path);

  if (parse_instance.is_class_file()) {
    //  Traverse superclasses
    assert(parse_instance.file_entry_class_def.is_valid());
    bool super_success = traverse_superclasses(parse_instance.file_entry_class_def, pipe_instance, source_data);
    if (!super_success) {
      return nullptr;
    }
  }

  bool import_success = traverse_imports(parse_instance.pending_type_imports, pipe_instance, source_data);
  if (!import_success) {
    return nullptr;
  }

  return root_res;
}

ParseSourceData FileScanSuccess::to_parse_source_data() const {
  const auto contents = file_contents.get();
  return ParseSourceData(*contents, &file_descriptor, &scan_info.row_column_indices);
}

}
