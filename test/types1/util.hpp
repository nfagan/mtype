#pragma once

#include "mt/mt.hpp"
#include <fstream>
#include <vector>

namespace mt {

struct Token;

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
  std::string file_contents;
  ScanInfo scan_info;
};

using FileScanResult = Result<FileScanError, FileScanSuccess>;

struct FileParseSuccess {
  FileParseSuccess() = default;

  FileParseSuccess(BoxedRootBlock root) : root_block(std::move(root)) {
    //
  }

  BoxedRootBlock root_block;
};

struct FileParseError {
  enum class Type {
    error_unspecified
  };

  explicit FileParseError(ParseErrors&& errs) :
  type(Type::error_unspecified), errors(std::move(errs)) {
    //
  }

  FileParseError() : type(Type::error_unspecified) {
    //
  }

  Type type;
  ParseErrors errors;
};

using FileParseResult = Result<FileParseError, FileParseSuccess>;

FileScanResult scan_file(const std::string& file_path);
FileParseResult parse_file(const std::vector<Token>& tokens, std::string_view contents, AstGenerator::ParseInputs& inputs);

}