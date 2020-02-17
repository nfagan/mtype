#include "mt/mt.hpp"
#include <fstream>
#include <thread>
#include <chrono>

namespace {

void run_parse(std::mutex& parse_barrier_mutex,
               int thread_index,
               std::vector<mt::BoxedRootBlock>& out_root_blocks,
               const std::vector<mt::Token>& tokens,
               std::string_view contents,
               mt::AstGenerator::ParseInputs& parse_inputs) {

  mt::AstGenerator ast_gen;
  auto parse_result = ast_gen.parse(tokens, contents, parse_inputs);
  if (!parse_result) {
    return;
  }

  {
    std::unique_lock<std::mutex> lock(parse_barrier_mutex);
    std::cout << "Finished parsing: " << thread_index << std::endl;
  }

  mt::IdentifierClassifier classifier(parse_inputs.string_registry,
                                      parse_inputs.store,
                                      contents);

  auto root_block = std::move(parse_result.value);
  classifier.transform_root(root_block);

  {
    std::unique_lock<std::mutex> lock(parse_barrier_mutex);
    out_root_blocks[thread_index] = std::move(root_block);
  }
}

}

int main(int argc, char** argv) {
  std::string file_path;

  if (argc < 2) {
    file_path = "/Users/Nick/Documents/MATLAB/repositories/fieldtrip/ft_databrowser.m";
  } else {
    file_path = argv[1];
  }

  std::ifstream ifs(file_path);
  if (!ifs) {
    std::cout << "Failed to open file: " << file_path << std::endl;
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
  auto tokens = std::move(scan_info.tokens);

  auto insert_res = insert_implicit_expr_delimiters(tokens, contents);
  if (insert_res) {
    insert_res.value().show();
    return 0;
  }

  mt::StringRegistry string_registry;
  mt::Store store;
  mt::AstGenerator::ParseInputs parse_inputs(&string_registry, &store, scan_info.functions_are_end_terminated);

  std::mutex parse_barrier_mutex;
  const int num_threads = 10;

  std::vector<std::thread> threads;
  std::vector<mt::BoxedRootBlock> root_blocks(num_threads);

  auto thread_t0 = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([&, i]() {
      run_parse(parse_barrier_mutex, i, root_blocks, tokens, contents, parse_inputs);
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto thread_t1 = std::chrono::high_resolution_clock::now();
  auto thread_elapsed = std::chrono::duration<double>(thread_t1 - thread_t0).count() * 1e3;

  std::vector<std::string> string_res;

  for (const auto& root_block : root_blocks) {
    mt::StringVisitor visitor(&string_registry, &store);
    visitor.include_def_ptrs = false;
    string_res.emplace_back(root_block->accept(visitor));
  }

  if (num_threads > 0) {
    const auto& str = string_res[0];

    for (int i = 1; i < num_threads; i++) {
      if (string_res[i] != str) {
        std::cout << "Mismatch: First size: " << str.size() << "; Sec size: " << string_res[i].size() << std::endl;
        std::cout << string_res[i] << std::endl;
      }
    }
  }

  mt::StringRegistry serial_string_registry;
  mt::Store serial_store;
  std::vector<mt::BoxedRootBlock> serial_root_blocks(num_threads);
  mt::AstGenerator::ParseInputs serial_parse_inputs(&serial_string_registry, &serial_store,
    scan_info.functions_are_end_terminated);

  auto serial_t0 = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_threads; i++) {
    run_parse(parse_barrier_mutex, i, serial_root_blocks, tokens, contents, serial_parse_inputs);
  }

  auto serial_t1 = std::chrono::high_resolution_clock::now();
  auto serial_elapsed = std::chrono::duration<double>(serial_t1 - serial_t0).count() * 1e3;

  std::cout << "Serial time: " << serial_elapsed << "(ms)" << std::endl;
  std::cout << "Parallel time: " << thread_elapsed << "(ms)" << std::endl;

  std::cout << "Serial size: " << serial_string_registry.size() << std::endl;
  std::cout << "Parallel size: " << string_registry.size() << std::endl;

  return 0;
}