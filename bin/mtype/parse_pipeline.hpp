#pragma once

#include "mt/mt.hpp"
#include "ast_store.hpp"

namespace mt {

struct FileScanError {
  enum class Type {
    error_unspecified,
    error_file_io,
    error_non_utf8_source,
    error_scan_failure,
  };

  FileScanError() : type(Type::error_unspecified) {
    //
  }

  FileScanError(Type type) : type(type) {
    //
  }

  FileScanError(ScanErrors&& errors) : type(Type::error_scan_failure), errors(std::move(errors)) {
    //
  }

  Type type;
  ScanErrors errors;
};

struct FileScanSuccess {
  FileScanSuccess() = default;
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(FileScanSuccess)

  ParseSourceData to_parse_source_data() const;

  std::unique_ptr<std::string> file_contents;
  CodeFileDescriptor file_descriptor;
  ScanInfo scan_info;
};

void swap(FileScanSuccess& a, FileScanSuccess& b);

using FileScanResult = Result<FileScanError, FileScanSuccess>;

namespace cmd {
  struct Arguments;
}

using ScanResultStore = std::unordered_map<FilePath, std::unique_ptr<FileScanSuccess>, FilePath::Hash>;

struct ParsePipelineInstanceData {
  ParsePipelineInstanceData(const SearchPath& search_path,
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
                            ParseErrors& parse_warnings);

  void add_error(const ParseError& err);
  void add_errors(const ParseErrors& errs);
  void add_warnings(const ParseErrors& warnings);

  void add_root(const FilePath& file_path, RootBlock* root_block);
  void remove_root(const FilePath& file_path);

  const SearchPath& search_path;
  Store& store;
  TypeStore& type_store;
  Library& library;
  StringRegistry& string_registry;
  AstStore& ast_store;
  ScanResultStore& scan_results;
  FunctionsByFile& functions_by_file;
  TokenSourceMap& source_data_by_token;
  std::unordered_set<RootBlock*> roots;
  std::unordered_set<FilePath, FilePath::Hash> root_files;
  const cmd::Arguments& arguments;

  ParseErrors& parse_errors;
  ParseErrors& parse_warnings;
};

AstStore::Entry* file_entry(ParsePipelineInstanceData& pipe_instance, const FilePath& file_path,
                            OnBeforeParse on_before_parse);
AstStore::Entry* file_entry(ParsePipelineInstanceData& pipe_instance, const FilePath& file_path);

}