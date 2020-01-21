#include <iostream>
#include <fstream>
#include <string>

#include "mt/mt.hpp"

namespace {
  void show_tokens(const std::vector<mt::Token>& tokens) {
    for (const auto& tok : tokens) {
      std::cout << tok << std::endl;
    }
  }
}

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

  auto scan_info = std::move(scan_result.value);
  std::cout << "Funcs are end terminated ? " << scan_info.functions_are_end_terminated << std::endl;
//  show_tokens(scan_info.tokens);

  auto insert_res = insert_implicit_expr_delimiters_in_groupings(scan_info.tokens, contents);
  if (insert_res) {
    insert_res.value().show();
    return 0;
  }

  mt::AstGenerator ast_gen;
  auto parse_result = ast_gen.parse(scan_info.tokens, contents, scan_info.functions_are_end_terminated);

  if (!parse_result) {
    for (const auto& err : parse_result.error) {
      err.show();
    }

    return 0;
  }

  mt::StringVisitor visitor;
  visitor.parenthesize_exprs = true;

  std::cout << parse_result.value->accept(visitor) << std::endl;

  return 0;
}