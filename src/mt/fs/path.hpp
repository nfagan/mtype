#pragma once

#include <string>
#include <iostream>
#include "../utility.hpp"

namespace mt {

class FilePath {
public:
  FilePath() = default;
  explicit FilePath(const std::string& component) : component(component) {
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

private:
  std::string component;
};

}