#include "util.hpp"
#include "mt/mt.hpp"

namespace mt {

FileParseResult parse_file(const std::vector<Token>& tokens, std::string_view contents, AstGenerator::ParseInputs& inputs) {
  mt::AstGenerator ast_gen;
  auto parse_result = ast_gen.parse(tokens, contents, inputs);
  if (!parse_result) {
    return make_error<FileParseError, FileParseSuccess>(std::move(parse_result.error));
  }

  mt::IdentifierClassifier classifier(inputs.string_registry, inputs.store, contents);

  auto root_block = std::move(parse_result.value);
  classifier.transform_root(root_block);

  auto& errs = classifier.get_errors();
  if (!errs.empty()) {
    return make_error<FileParseError, FileParseSuccess>(std::move(errs));
  }

  return make_success<FileParseError, FileParseSuccess>(std::move(root_block));
}

FileScanResult scan_file(const std::string& file_path) {
  using std::swap;

  std::ifstream ifs(file_path);
  if (!ifs) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_file_io);
  }

  std::string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

  if (!mt::utf8::is_valid(contents)) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_non_utf8_source);
  }

  mt::Scanner scanner;
  auto scan_result = scanner.scan(contents);

  if (!scan_result) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_scan_failure);
  }

  auto scan_info = std::move(scan_result.value);

  auto insert_res = insert_implicit_expr_delimiters(scan_info.tokens, contents);
  if (insert_res) {
    return make_error<FileScanError, FileScanSuccess>(FileScanError::Type::error_scan_failure);
  }

  FileScanResult success;
  swap(success.value.scan_info, scan_info);
  swap(success.value.file_contents, contents);

  return success;
}


}
