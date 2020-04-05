#include "mt/mt.hpp"
#include "mt/search_path.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

namespace {

std::string read_path_text_file(bool* success) {
  auto path_file_path = mt::fs::join(mt::FilePath(MT_MATLAB_DATA_DIR), mt::FilePath("path.txt"));

  std::ifstream ifs(path_file_path.c_str());
  if (!ifs) {
    *success = false;
    return {};
  }

  std::string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

  if (!mt::utf8::is_valid(contents)) {
    *success = false;
    return {};
  }

  *success = true;
  return contents;
}
}

int main(int argc, char** argv) {
  using namespace mt;
  bool success;
  std::string file_contents = read_path_text_file(&success);
  if (!success) {
    std::cout << "Failed to read paths text file." << std::endl;
    return 0;
  }

  const auto split_paths = split(file_contents, Character('\n'));
  std::vector<FilePath> directories;
  directories.reserve(split_paths.size());

  for (const auto& p : split_paths) {
    directories.emplace_back(std::string(p));
  }

  SearchPath search_path(std::move(directories));

  auto t0 = std::chrono::high_resolution_clock::now();
  const auto status = search_path.build();
  auto t1 = std::chrono::high_resolution_clock::now();

  if (status != DirectoryIterator::Status::success) {
    std::cout << "Failed to build search path." << std::endl;
    return 0;
  }

  auto dur = std::chrono::duration<double>(t1 - t0).count() * 1e3;
  std::cout << "Ok: built search path in " << dur << " ms." << std::endl;
  std::cout << "Found " << search_path.size() << " unique identifiers." << std::endl;

  FilePath candidate_dir = fs::join(FilePath(MT_MATLAB_TEST_DIR), FilePath("test_private"));
  std::cout << candidate_dir << std::endl;

  auto lookup_res = search_path.search_for("test_priv", candidate_dir);
  if (lookup_res) {
    std::cout << "Found function: " << lookup_res.value()->defining_file << std::endl;
  } else {
    std::cout << "Failed to find function." << std::endl;
  }

  FilePath s("a/béé/c/f/gé");
  std::cout << "Full path: " << s << "; Dir name: " << fs::directory_name(s) << std::endl;

  return 0;
}