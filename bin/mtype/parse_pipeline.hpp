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

  Type type;
};

struct FileScanSuccess {
  FileScanSuccess() = default;
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(FileScanSuccess)

  std::unique_ptr<std::string> file_contents;
  CodeFileDescriptor file_descriptor;
  ScanInfo scan_info;
};

void swap(FileScanSuccess& a, FileScanSuccess& b);

using FileScanResult = Result<FileScanError, FileScanSuccess>;

namespace cmd {
  struct Arguments;
}

using ScanResultStore = std::vector<std::unique_ptr<FileScanSuccess>>;

struct ParsePipelineInstanceData {
  ParsePipelineInstanceData(const SearchPath& search_path,
                            Store& store,
                            TypeStore& type_store,
                            StringRegistry& str_registry,
                            AstStore& ast_store,
                            ScanResultStore& scan_results,
                            TokenSourceMap& source_data_by_token,
                            const cmd::Arguments& arguments);

  const SearchPath& search_path;
  Store& store;
  TypeStore& type_store;
  StringRegistry& string_registry;
  AstStore& ast_store;
  ScanResultStore& scan_results;
  TokenSourceMap& source_data_by_token;
  std::unordered_set<RootBlock*> roots;
  const cmd::Arguments& arguments;
};

AstStore::Entry* file_entry(ParsePipelineInstanceData& pipe_instance, const FilePath& file_path);

}