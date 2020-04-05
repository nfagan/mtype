#pragma once

#include <string>
#include <iostream>
#include "../utility.hpp"

namespace mt {

class FilePath {
public:
  struct Hash {
    std::size_t operator()(const FilePath& a) const;
  };
public:
  FilePath() = default;
  explicit FilePath(std::string component) : component(std::move(component)) {
    //
  }

  ~FilePath() = default;
  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(FilePath)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(FilePath)

  bool empty() const {
    return component.empty();
  }

  const char* c_str() const {
    return component.c_str();
  }

  const std::string& str() const {
    return component;
  }

  inline friend std::ostream& operator<<(std::ostream& stream, const FilePath& file_path) {
    return stream << file_path.component;
  }

  inline friend bool operator==(const FilePath& a, const FilePath& b) {
    return a.component == b.component;
  }

  inline friend bool operator!=(const FilePath& a, const FilePath& b) {
    return a.component != b.component;
  }

private:
  std::string component;
};

namespace fs {
  extern const char* const separator;

  FilePath join(const FilePath& a, const FilePath& b);
  FilePath directory_name(const FilePath& a);
}

}