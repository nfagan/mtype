#include "mt/mt.hpp"
#include "typing.hpp"
#include "test_cases.hpp"
#include "util.hpp"
#include <chrono>

namespace {
std::string get_source_file_path(int argc, char** argv) {
  if (argc < 2) {
//    return "/Users/Nick/Documents/MATLAB/repositories/fieldtrip/ft_databrowser.m";
    return "/Users/Nick/repositories/cpp/mt/matlab/test/X.m";
  } else {
    return argv[1];
  }
}
}

int main(int argc, char** argv) {
  using namespace mt;

  const auto file_path = get_source_file_path(argc, argv);
  const auto scan_result = scan_file(file_path);
  if (!scan_result) {
    std::cout << "Failed to scan file." << std::endl;
    return -1;
  }

  auto& scan_info = scan_result.value.scan_info;
  const auto& tokens = scan_info.tokens;
  const auto& contents = scan_result.value.file_contents;

  Store store;
  StringRegistry str_registry;
  AstGenerator::ParseInputs parse_inputs(&str_registry, &store, scan_info.functions_are_end_terminated);

  auto parse_result = parse_file(tokens, contents, parse_inputs);
  if (!parse_result) {
    ShowParseErrors show(&scan_info.row_column_indices);
    show.is_rich_text = true;
    show.show(parse_result.error.errors);
    std::cout << "Failed to parse file." << std::endl;
    return -1;
  }

  auto t0 = std::chrono::high_resolution_clock::now();

  TypeVisitor type_visitor(store, str_registry);
  std::unique_ptr<const RootBlock> root_block = std::move(parse_result.value.root_block);
  root_block->accept_const(type_visitor);

  mt::run_all();

  auto t1 = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration<double>(t1 - t0).count() * 1e3;
  std::cout << elapsed << " (ms)" << std::endl;

//  StringVisitor str_visitor(&str_registry, &store);
//  std::cout << root_block->accept(str_visitor) << std::endl;

  return 0;
}