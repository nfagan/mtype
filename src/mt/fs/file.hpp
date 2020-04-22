#pragma once

#include <string>
#include <memory>

namespace mt {
  template <typename T>
  class Optional;

  class FilePath;
}

namespace mt::fs {
  Optional<std::unique_ptr<std::string>> read_file(const FilePath& path);
  bool file_exists(const FilePath& path);
}