#pragma once

#include <string>

namespace mt {

class FilePath {
public:
  FilePath() = default;
  explicit FilePath(const std::string& component) : component(component) {
    //
  }

  ~FilePath() = default;

  bool empty() const {
    return component.empty();
  }

  const char* c_str() const {
    return component.c_str();
  }

  const std::string& str() const {
    return component;
  }

private:
  std::string component;
};

}