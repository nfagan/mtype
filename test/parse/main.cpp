#include <iostream>
#include <fstream>
#include <string>

#include "mt/mt.hpp"

int main(int argc, char** argv) {
  std::string file_path;

  if (argc < 2) {
    file_path = "/Users/Nick/Documents/MATLAB/repositories/mt/test/parse7.m";
  } else {
    file_path = argv[1];
  }

  std::ifstream ifs(file_path);
  if (!ifs) {
    std::cout << "Failed to open file" << std::endl;
    return 0;
  }

  std::string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

  if (!mt::utf8::is_valid(contents)) {
    std::cout << "Source file is non-ascii or non-utf8." << std::endl;
    return 0;
  }

  mt::Scanner scanner;
  auto scan_result = scanner.scan(contents);

  if (!scan_result) {
    for (const auto& err : scan_result.error.errors) {
      std::cout << err.message << std::endl;
    }

    return 0;
  }

  mt::AstGenerator ast_gen;
  auto parse_result = ast_gen.parse(scan_result.value, contents);

  if (!parse_result) {
    for (const auto& err : parse_result.error) {
      err.show();
    }

    return 0;
  }

  mt::StringVisitor visitor;
  visitor.parenthesize_exprs = false;

  std::cout << parse_result.value->accept(visitor) << std::endl;
}