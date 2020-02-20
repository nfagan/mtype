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

//  {
//    std::unique_lock<std::mutex> lock(parse_barrier_mutex);
//    std::cout << "Finished parsing: " << thread_index << std::endl;
//  }

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

struct FileScanResult {
  FileScanResult() : success(false), functions_are_end_terminated(true) {
    //
  }

  std::string contents;
  std::vector<mt::Token> tokens;
  bool success;
  bool functions_are_end_terminated;
};

mt::Optional<std::vector<mt::FilePath>> find_n_files(const mt::FilePath& in_directory, int64_t num) {
  using namespace mt;

  DirectoryIterator iterator(in_directory);

  auto res = iterator.open();
  if (res != DirectoryIterator::Status::success) {
    return mt::NullOpt{};
  }

  std::vector<mt::FilePath> file_paths;
  const std::string src_path{in_directory.c_str()};

  while (true) {
    const auto entry_res = iterator.next();
    if (!entry_res) {
      return NullOpt{};
    } else if (!entry_res.value) {
      break;
    }

    const auto& dir_entry = entry_res.value.value();

    if (dir_entry.name_size <= 2) {
      continue;
    }

    const std::size_t len_minus_ext = dir_entry.name_size - 2;
    const auto* maybe_dot_m = &dir_entry.name[len_minus_ext];
    if (maybe_dot_m[0] != '.' || maybe_dot_m[1] != 'm') {
      continue;
    }

    std::string string_path{dir_entry.name};

    FilePath joined_path(src_path + "/" + string_path);
    file_paths.emplace_back(joined_path);

    if (file_paths.size() == num) {
      break;
    }
  }

  if (file_paths.size() < num) {
    return NullOpt{};
  }

  return Optional<std::vector<mt::FilePath>>(std::move(file_paths));
}

FileScanResult scan_file(const std::string& file_path) {
  FileScanResult result;

  std::ifstream ifs(file_path);
  if (!ifs) {
    std::cout << "Failed to open file: " << file_path << std::endl;
    return result;
  }

  std::string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

  if (!mt::utf8::is_valid(contents)) {
    std::cout << "Source file is non-ascii or non-utf8." << std::endl;
    return result;
  }

  mt::Scanner scanner;
  auto scan_result = scanner.scan(contents);

  if (!scan_result) {
    return result;
  }

  auto scan_info = std::move(scan_result.value);
  auto tokens = std::move(scan_info.tokens);

  auto insert_res = insert_implicit_expr_delimiters(tokens, contents);
  if (insert_res) {
    return result;
  }

  std::swap(result.contents, contents);
  std::swap(result.tokens, tokens);
  result.success = true;
  result.functions_are_end_terminated = scan_info.functions_are_end_terminated;

  return result;
}

}

int main(int argc, char** argv) {
  std::string src_dir;

  if (argc < 2) {
    src_dir = "/Users/Nick/Documents/MATLAB/repositories/fieldtrip";
//    src_dir = "/Users/Nick/repositories/cpp/mt/matlab/test";
  } else {
    src_dir = argv[1];
  }

  const int num_threads = 32;

  auto file_path_res = find_n_files(mt::FilePath(src_dir), num_threads);
  if (!file_path_res) {
    std::cout << "Failed to acquire at least " << num_threads << " files.";
    return 0;
  }

  const auto file_paths = std::move(file_path_res.rvalue());
  std::vector<FileScanResult> scan_results(num_threads);

  for (int i = 0; i < num_threads; i++) {
    const auto& file_path = file_paths[i];
    auto scan_result = scan_file(file_path.str());
    if (!scan_result.success) {
      return 0;
    }

    std::swap(scan_results[i], scan_result);
  }

  mt::StringRegistry string_registry;
  mt::Store store;

  std::mutex parse_barrier_mutex;

  std::vector<std::thread> threads;
  std::vector<mt::BoxedRootBlock> root_blocks(num_threads);

  auto thread_t0 = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([&, i]() {
      const auto& tokens = scan_results[i].tokens;
      const auto& contents = scan_results[i].contents;
      const bool is_end_terminated = scan_results[i].functions_are_end_terminated;

      mt::AstGenerator::ParseInputs parse_inputs(&string_registry, &store, is_end_terminated);

      run_parse(parse_barrier_mutex, i, root_blocks, tokens, contents, parse_inputs);
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto thread_t1 = std::chrono::high_resolution_clock::now();
  auto thread_elapsed = std::chrono::duration<double>(thread_t1 - thread_t0).count() * 1e3;

  std::vector<std::string> string_res(root_blocks.size());

  for (int i = 0; i < num_threads; i++) {
    if (root_blocks[i]) {
      mt::StringVisitor visitor(&string_registry, &store);
      visitor.include_def_ptrs = false;
      string_res[i] = root_blocks[i]->accept(visitor);
    }
  }

  mt::StringRegistry serial_string_registry;
  mt::Store serial_store;
  std::vector<mt::BoxedRootBlock> serial_root_blocks(num_threads);

  auto serial_t0 = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_threads; i++) {
    const auto& tokens = scan_results[i].tokens;
    const auto& contents = scan_results[i].contents;

    mt::AstGenerator::ParseInputs serial_parse_inputs(&serial_string_registry, &serial_store,
                                                      scan_results[i].functions_are_end_terminated);

    run_parse(parse_barrier_mutex, i, serial_root_blocks, tokens, contents, serial_parse_inputs);
  }

  auto serial_t1 = std::chrono::high_resolution_clock::now();
  auto serial_elapsed = std::chrono::duration<double>(serial_t1 - serial_t0).count() * 1e3;
  bool first_mismatch = true;

  for (int i = 0; i < num_threads; i++) {
    std::string serial_res;

    if (serial_root_blocks[i]) {
      mt::StringVisitor visitor(&serial_string_registry, &serial_store);
      visitor.include_def_ptrs = false;
      serial_res = serial_root_blocks[i]->accept(visitor);
    }

    if (serial_res != string_res[i]) {
      std::cout << "Mismatch: " << file_paths[i].str() << std::endl;
      if (first_mismatch) {
        std::cout << "Serial result ========================================" << std::endl << std::endl;
        std::cout << serial_res << std::endl << std::endl;
        std::cout << "Parallel result ======================================" << std::endl << std::endl;
        std::cout << string_res[i] << std::endl;
        std::cout << "========================================" << std::endl;
        first_mismatch = false;
      }
    }
  }

  std::cout << "Serial time: " << serial_elapsed << "(ms)" << std::endl;
  std::cout << "Parallel time: " << thread_elapsed << "(ms)" << std::endl;

  std::cout << "Serial size: " << serial_string_registry.size() << std::endl;
  std::cout << "Parallel size: " << string_registry.size() << std::endl;

  return 0;
}