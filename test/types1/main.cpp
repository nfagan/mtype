#include "mt/mt.hpp"
#include "util.hpp"
#include <chrono>

namespace {
std::string get_source_file_path(int argc, char** argv) {
  if (argc < 2) {
    return "/Users/Nick/Documents/MATLAB/repositories/fieldtrip/ft_databrowser.m";
  } else {
    return argv[1];
  }
}
}

namespace mt {
class TypeVisitor : public TypePreservingVisitor {
public:
  void root_block(RootBlock& block) override {
    std::cout << "root" << std::endl;
  }
};
}

int main(int argc, char** argv) {
  using namespace mt;

  auto t0 = std::chrono::high_resolution_clock::now();

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
  StringRegistry string_registry;
  AstGenerator::ParseInputs parse_inputs(&string_registry, &store, scan_info.functions_are_end_terminated);

  auto parse_result = parse_file(tokens, contents, parse_inputs);
  if (!parse_result) {
    show_parse_errors(parse_result.error.errors);
    std::cout << "Failed to parse file." << std::endl;
    return -1;
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration<double>(t1 - t0).count() * 1e3;
  std::cout << elapsed << " (ms)" << std::endl;

  TypeVisitor visitor;
  parse_result.value.root_block->accept(visitor);

  return 0;
}