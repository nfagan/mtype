#include "mt/mt.hpp"
#include "typing.hpp"
#include "test_cases.hpp"
#include "library.hpp"
#include "util.hpp"
#include "type_representation.hpp"
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
  const auto& file_descriptor = scan_result.value.file_descriptor;

  Store store;
  StringRegistry str_registry;
  AstGenerator::ParseInputs parse_inputs(&str_registry, &store,
                                         &file_descriptor, scan_info.functions_are_end_terminated);

  auto parse_result = parse_file(tokens, contents, parse_inputs);
  if (!parse_result) {
    ShowParseErrors show(&scan_info.row_column_indices);
    show.is_rich_text = true;
    show.show(parse_result.error.errors);
    std::cout << "Failed to parse file." << std::endl;
    return -1;
  }

  auto t0 = std::chrono::high_resolution_clock::now();

  //  Reserve space
  TypeStore type_store(10000);
  TypeEquality type_eq(type_store);
  Library library(type_store, type_eq, str_registry);
  library.make_known_types();

  Unifier unifier(type_store, library, str_registry);

  TypeVisitor type_visitor(unifier, store, type_store, library, str_registry);
  std::unique_ptr<const RootBlock> root_block = std::move(parse_result.value.root_block);
  root_block->accept_const(type_visitor);

  auto unify_res = unifier.unify();

  mt::run_all();

  auto t1 = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration<double>(t1 - t0).count() * 1e3;
  std::cout << elapsed << " (ms)" << std::endl;

  type_visitor.show_type_distribution();
  type_visitor.show_variable_types();

  if (unify_res.is_error()) {
    TypeToString type_to_string(type_store, library, &str_registry);
    type_to_string.explicit_destructured_tuples = false;
    ShowUnificationErrors show(type_store, type_to_string);
    show.show(unify_res.simplify_failures, contents, file_descriptor);

    if (unify_res.simplify_failures.empty()) {
      std::cout << "ERROR" << std::endl;
    }
  }

//  StringVisitor str_visitor(&str_registry, &store);
//  std::cout << root_block->accept(str_visitor) << std::endl;

  return 0;
}