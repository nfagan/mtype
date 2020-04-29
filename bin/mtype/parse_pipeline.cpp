#include "parse_pipeline.hpp"
#include "ast_store.hpp"
#include "command_line.hpp"
#include <fstream>

namespace mt {
namespace {

void parse_file(ParseInstance* instance, const std::vector<Token>& tokens) {
  AstGenerator ast_gen(instance, tokens);

  instance->on_before_parse(ast_gen, *instance);
  ast_gen.parse();

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
  const auto maybe_function_def_node = parse_instance.file_entry_function_def_node;
  const auto file_type = parse_instance.file_type;

  AstStore::Entry entry(std::move(root_block), maybe_class_def,
                        maybe_function_def, maybe_function_def_node, file_type);

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

ParseError make_error_failed_to_extract_method_definition_from_file(const ParseSourceData& source_data,
                                                                    const Token& token,
                                                                    const std::string& file) {
  std::string msg = "Failed to extract function definition from file: `" + file + "`.";
  return ParseError(source_data.source, token, msg, source_data.file_descriptor);
}

ParseError make_error_duplicate_method_type_annotation(const ParseSourceData& source_data,
                                                       const Token& token) {
  const auto msg = "Duplicate type annotation.";
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

BoxedMethodNode extract_external_method(ParsePipelineInstanceData& pipe_instance,
                                        const ParseSourceData& source_data,
                                        const std::string& file_str,
                                        PendingExternalMethod& method,
                                        AstStore::Entry* root_entry) {
  auto block = std::move(root_entry->root_block->block);
  const int64_t num_nodes = block->nodes.size();

  BoxedFunctionDefNode function_def_node;
  BoxedType maybe_type;
  bool extracted_definition = false;
  const auto entry_def_node = root_entry->file_entry_function_def_node;

  for (int64_t i = 0; i < num_nodes; i++) {
    auto& node = block->nodes[i];
    auto maybe_function_def_node = node->extract_function_def_node();

    if (maybe_function_def_node && maybe_function_def_node.value() == entry_def_node) {
      auto node_ptr = static_cast<FunctionDefNode*>(node.release());
      function_def_node = std::unique_ptr<FunctionDefNode>(node_ptr);
      extracted_definition = true;
      block->nodes.erase(block->nodes.begin() + i);
      break;
    }

    auto maybe_macro = node->extract_type_annot_macro();
    if (maybe_macro) {
      auto maybe_assert = maybe_macro.value()->annotation->extract_type_assertion();
      if (maybe_assert &&
          maybe_assert.value()->node->is_function_def_node() &&
          static_cast<const FunctionDefNode*>(maybe_assert.value()->node.get()) == entry_def_node) {
        //  A type was provided both in the header file and in the definition.
        if (method.method_declaration.maybe_type) {
          auto err = make_error_duplicate_method_type_annotation(source_data, method.name_token);
          pipe_instance.add_error(err);
          return nullptr;
        }

        auto node_ptr = static_cast<FunctionDefNode*>(maybe_assert.value()->node.release());
        function_def_node = std::unique_ptr<FunctionDefNode>(node_ptr);
        maybe_type = std::move(maybe_assert.value()->has_type);
        extracted_definition = true;
        //  Erase this node from the block.
        block->nodes.erase(block->nodes.begin() + i);
        break;
      }
    }
  }

  if (!extracted_definition) {
    auto err =
      make_error_failed_to_extract_method_definition_from_file(source_data, method.name_token, file_str);
    pipe_instance.add_error(err);
    return nullptr;
  } else if (method.method_declaration.maybe_type) {
    maybe_type = std::move(method.method_declaration.maybe_type);
  }

  auto method_node =
    std::make_unique<MethodNode>(std::move(function_def_node), std::move(maybe_type), nullptr);
  //  Add the remaining nodes to this method.
  method_node->external_block = std::move(block);

  return method_node;
}

BoxedMethodNode defined_external_method(ParsePipelineInstanceData& pipe_instance,
                                        const ParseSourceData& source_data,
                                        PendingExternalMethod& method,
                                        const FilePath& expect_method_file) {
  const auto on_before_parse = [&](AstGenerator& gen, ParseInstance& instance) {
    //  Mark that the root block is an external method.
    instance.treat_root_as_external_method = true;
    //  Jump into scope.
    gen.enter_methods_block_via_external_method(method);
  };

  const auto res = file_entry(pipe_instance, expect_method_file, on_before_parse);
  if (!res) {
    return nullptr;
  }

  const auto& file_str = expect_method_file.str();

  if (res->file_type != CodeFileType::function_def) {
    //  File must be a function file.
    auto err =
      make_error_failed_to_extract_method_definition_from_file(source_data, method.name_token, file_str);
    pipe_instance.add_error(err);
    return nullptr;
  }

  auto method_node = extract_external_method(pipe_instance, source_data, file_str, method, res);

  //  Because we moved the FunctionDefNode from `res->root_block`, the root block of the AST is
  //  invalid. We need to erase its entry in the ast store.
  pipe_instance.remove_root(expect_method_file);

  return method_node;
}

BoxedMethodNode abstract_undefined_external_method(ParsePipelineInstanceData& pipe_instance,
                                                   const ParseSourceData& source_data,
                                                   PendingExternalMethod& method,
                                                   const FilePath& expect_method_file) {
  auto def_node = std::make_unique<FunctionDefNode>(method.name_token,
    method.method_declaration.pending_def_handle,
    method.method_declaration.pending_ref_handle,
    method.matlab_scope,
    method.type_scope
  );

  auto method_node = std::make_unique<MethodNode>(std::move(def_node),
    std::move(method.method_declaration.maybe_type), nullptr);

  return method_node;
}

bool traverse_external_method(ParsePipelineInstanceData& pipe_instance,
                              const ParseSourceData& source_data,
                              const FilePath& class_directory_path,
                              PendingExternalMethod& method) {

  const auto method_id = method.method_name.full_name();
  auto method_file_name = pipe_instance.string_registry.at(method_id) + ".m";
  const auto expect_method_file = fs::join(class_directory_path, FilePath(method_file_name));
  const auto& file_str = expect_method_file.str();

  const bool exists = fs::file_exists(expect_method_file);
  BoxedMethodNode method_node;

  if (exists) {
    method_node =
      defined_external_method(pipe_instance, source_data, method, expect_method_file);
    if (!method_node) {
      return false;
    }

  } else if (method.method_attributes.is_abstract()) {
    method_node =
      abstract_undefined_external_method(pipe_instance, source_data, method, expect_method_file);
    if (!method_node) {
      return false;
    }

  } else {
    //  Could not resolve method declaration, and method is not marked as abstract.
    pipe_instance.add_error(make_error_unresolved_file(source_data, method.name_token, file_str));
    return false;
  }

  assert(method_node);
  //  Add the new method node to the ClassDefNode.
  method.class_node->method_defs.push_back(std::move(method_node));
  return true;
}

bool traverse_external_methods(ParsePipelineInstanceData& pipe_instance,
                               const ParseSourceData& source_data,
                               const FilePath& class_directory_path,
                               PendingExternalMethods& external_methods) {
  bool success = true;

  for (auto& method : external_methods) {
    bool tmp_success =
      traverse_external_method(pipe_instance, source_data, class_directory_path, method);
    if (!tmp_success) {
      success = false;
    }
  }

  return success;
}

void on_before_parse_no_op(AstGenerator&, ParseInstance&) {
  //
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
                                                     FunctionsByFile& functions_by_file,
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
  functions_by_file(functions_by_file),
  source_data_by_token(source_data_by_token),
  arguments(arguments),
  parse_errors(parse_errors),
  parse_warnings(parse_warnings) {
  //
}

void ParsePipelineInstanceData::add_root(const FilePath& file_path, RootBlock* root_block) {
  roots.insert(root_block);
  root_files.insert(file_path);
}

void ParsePipelineInstanceData::require_root(const FilePath& file_path, RootBlock* root_block) {
  if (root_files.count(file_path) == 0) {
    add_root(file_path, root_block);
  }
}

void ParsePipelineInstanceData::remove_root(const FilePath& file_path) {
  auto maybe_entry = ast_store.lookup(file_path);
  assert(maybe_entry);

  auto root_block = maybe_entry->root_block.get();
  bool was_removed = ast_store.remove(file_path);
  assert(was_removed);
  was_removed = root_files.erase(file_path) > 0;
  assert(was_removed);
  was_removed = roots.erase(root_block) > 0;
  assert(was_removed);
}

std::vector<AstStore::Entry*> ParsePipelineInstanceData::gather_root_entries() const {
  std::vector<AstStore::Entry*> entries;
  for (const auto& root_file : root_files) {
    entries.emplace_back(ast_store.lookup(root_file));
  }
  return entries;
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
  return file_entry(pipe_instance, file_path, on_before_parse_no_op);
}

AstStore::Entry* file_entry(ParsePipelineInstanceData& pipe_instance,
                            const FilePath& file_path,
                            OnBeforeParse on_before_parse) {
  const auto maybe_ast_entry = pipe_instance.ast_store.lookup(file_path);
  if (maybe_ast_entry) {
    pipe_instance.require_root(file_path, maybe_ast_entry->root_block.get());
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
    &pipe_instance.string_registry, &pipe_instance.functions_by_file, source_data,
    scan_result->scan_info.functions_are_end_terminated, std::move(on_before_parse));

  const auto root_res = run_parse_file(parse_instance, *scan_result, pipe_instance);
  if (!root_res) {
    return nullptr;
  }

  pipe_instance.add_root(file_path, root_res->root_block.get());

  if (parse_instance.is_class_file()) {
    //  Traverse superclasses.
    assert(parse_instance.file_entry_class_def.is_valid());
    bool super_success =
      traverse_superclasses(parse_instance.file_entry_class_def, pipe_instance, source_data);

    if (!super_success) {
      return nullptr;
    }

    //  Traverse external methods.
    const auto class_dir_path = fs::directory_name(file_path);
    auto& external_methods = parse_instance.pending_external_methods;

    bool external_method_success =
      traverse_external_methods(pipe_instance, source_data, class_dir_path, external_methods);
    if (!external_method_success) {
      return nullptr;
    }
  }

  bool import_success =
    traverse_imports(parse_instance.pending_type_imports, pipe_instance, source_data);

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
