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

  std::vector<std::string> file_paths;
  for (const auto& path : split_paths) {
    auto res = read_path(path, file_paths);
    if (!res) {
      break;
    }
  }

  std::cout << "Num files: " << file_paths.size() << std::endl;

  return 0;
}