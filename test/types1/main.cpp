#include "mt/mt.hpp"
#include "util.hpp"
#include "test_cases.hpp"
#include <chrono>

namespace {

void show_parse_errors(const mt::ParseErrors& errs, const mt::TextRowColumnIndices& inds) {
  mt::ShowParseErrors show(&inds);
  show.is_rich_text = true;
  show.show(errs);
}

std::string get_source_file_path(int argc, char** argv) {
  if (argc < 2) {
//    return "/Users/Nick/Documents/MATLAB/repositories/fieldtrip/ft_databrowser.m";
//    return "/Users/Nick/repositories/cpp/mt/matlab/test/X.m";
    return "/Users/Nick/Desktop/stress3.m";
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
  AstGenerator::ParseInputs parse_inputs(&str_registry, &store, &file_descriptor,
    scan_info.functions_are_end_terminated);

  auto parse_result = parse_file(tokens, contents, parse_inputs);
  if (!parse_result) {
    show_parse_errors(parse_result.error.errors, scan_info.row_column_indices);
    std::cout << "Failed to parse file." << std::endl;
    return -1;

  } else if (!parse_result.value.warnings.empty()) {
    show_parse_errors(parse_result.value.warnings, scan_info.row_column_indices);
  }

  auto t0 = std::chrono::high_resolution_clock::now();

  //  Reserve space
  TypeStore type_store(1e5);
  Library library(type_store, str_registry);
  library.make_known_types();

  Substitution substitution;
  Unifier unifier(type_store, library, str_registry);
  TypeConstraintGenerator type_visitor(substitution, store, type_store, library, str_registry);

  std::unique_ptr<const RootBlock> root_block = std::move(parse_result.value.root_block);
  root_block->accept_const(type_visitor);

  auto unify_res = unifier.unify(&substitution);

  auto t1 = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration<double>(t1 - t0).count() * 1e3;
  std::cout << elapsed << " (ms)" << std::endl;

  TypeToString type_to_string(&library, &str_registry);
  type_to_string.explicit_destructured_tuples = false;
  type_to_string.arrow_function_notation = true;
  type_to_string.max_num_type_variables = 3;

//  type_visitor.show_type_distribution();
//  type_visitor.show_variable_types(type_to_string);
  type_visitor.show_local_function_types(type_to_string);

  if (unify_res.is_error()) {
    ShowUnificationErrors show(type_to_string);
    show.show(unify_res.errors, contents, file_descriptor, scan_info.row_column_indices);

    if (unify_res.errors.empty()) {
      std::cout << "ERROR" << std::endl;
    }
  }

  std::cout << "Num type eqs: " << substitution.num_type_equations() << std::endl;
  std::cout << "Subs size: " << substitution.num_bound_terms() << std::endl;
  std::cout << "Num types: " << type_store.size() << std::endl;
  std::cout << "Type size: " << sizeof(mt::Type) << std::endl;
  std::cout << "Vec size: " << sizeof(TypePtrs) << std::endl;

  mt::run_all();

//  StringVisitor str_visitor(&str_registry, &store);
//  std::cout << root_block->accept(str_visitor) << std::endl;

  return 0;
}