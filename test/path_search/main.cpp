#include "mt/mt.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <dirent.h>

namespace {

std::string read_path_text_file(bool* success) {
  auto path_file_path = std::string(MT_MATLAB_DATA_DIR) + "/path.txt";

  std::ifstream ifs(path_file_path);
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

bool read_path_directory_iterator(std::string_view dir_path, std::vector<std::string>& out_files) {
  using namespace mt;

  std::string string_path{dir_path};
  FilePath file_path(string_path);
  DirectoryIterator iterator(file_path);

  const auto status = iterator.open();
  if (status != DirectoryIterator::Status::success) {
    return false;
  }

  while (true) {
    auto next_entry_res = iterator.next();
    if (!next_entry_res) {
      return false;
    } else if (!next_entry_res.value) {
      break;
    }

    const auto& dir_entry = next_entry_res.value.value();
    const auto* name = dir_entry.name;

    if (dir_entry.name_size <= 2) {
      continue;
    }

    if (dir_entry.type == DirectoryEntry::Type::regular_file) {
      const std::size_t len_minus_ext = dir_entry.name_size - 2;
      auto* maybe_dot_m = &name[len_minus_ext];

      if (maybe_dot_m[0] == '.' && maybe_dot_m[1] == 'm') {
        std::string file_name{name, len_minus_ext};
        out_files.emplace_back(std::move(file_name));
      }

    } else if (dir_entry.type == DirectoryEntry::Type::directory && (name[0] == '+' || name[0] == '@')) {
      std::string joined_path{dir_path};
      joined_path += '/';
      joined_path += name;
      read_path_directory_iterator(joined_path, out_files);
    }
  }
  return true;
}

bool read_path(std::string_view dir_path, std::vector<std::string>& out_files) {
  std::string string_path{dir_path};

  DIR* dir = opendir(string_path.c_str());
  if (dir == nullptr) {
    return false;
  }

  struct dirent* dir_entry = nullptr;
  while (true) {
    dir_entry = readdir(dir);
    if (dir_entry == nullptr) {
      break;
    } else if (dir_entry->d_namlen <= 2) {
      continue;
    }

    const auto* name = dir_entry->d_name;

    //  > 2 for .m
    if (dir_entry->d_type == DT_REG) {
      const std::size_t len_minus_ext = dir_entry->d_namlen - 2;
      auto* maybe_dot_m = &name[len_minus_ext];

      if (maybe_dot_m[0] == '.' && maybe_dot_m[1] == 'm') {
        std::string file_name{dir_entry->d_name, len_minus_ext};
        out_files.emplace_back(std::move(file_name));
      }

    } else if (dir_entry->d_type == DT_DIR && (name[0] == '+' || name[0] == '@')) {
      std::string joined_path{dir_path};
      joined_path += '/';
      joined_path += dir_entry->d_name;
      read_path(joined_path, out_files);
    }
  }

  closedir(dir);
  return true;
}

}

int main(int argc, char** argv) {
  bool success;
  std::string file_contents = read_path_text_file(&success);
  if (!success) {
    return 0;
  }

  const auto split_paths = mt::split(file_contents, mt::Character('\n'));

  std::vector<std::string> native_file_paths;
  std::vector<std::string> my_file_paths;

  auto t0 = std::chrono::high_resolution_clock::now();

  for (const auto& path : split_paths) {
    auto res = read_path(path, native_file_paths);
    if (!res) {
      break;
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  const auto elapsed_native_ms = std::chrono::duration<double>(t1 - t0).count() * 1e3;

  t0 = std::chrono::high_resolution_clock::now();
  for (const auto& path : split_paths) {
    auto res = read_path_directory_iterator(path, my_file_paths);
    if (!res) {
      break;
    }
  }

  t1 = std::chrono::high_resolution_clock::now();
  const auto elapsed_my_ms = std::chrono::duration<double>(t1 - t0).count() * 1e3;

  std::cout << "Num native files: " << native_file_paths.size() << std::endl;
  std::cout << "Num my iterator files: " << my_file_paths.size() << std::endl;

  std::cout << "Native time: " << elapsed_native_ms << std::endl;
  std::cout << "My time: " << elapsed_my_ms << std::endl;

  if (my_file_paths.size() == native_file_paths.size()) {
    int64_t num_mismatch = 0;

    for (int64_t i = 0; i < my_file_paths.size(); i++) {
      if (my_file_paths[i] != native_file_paths[i]) {
        num_mismatch++;
      }
    }

    std::cout << "Num mismatching files: " << num_mismatch << std::endl;
  } else {
    std::cout << "Sizes mismatched between native and my directory iterator" << std::endl;
  }

  return 0;
}