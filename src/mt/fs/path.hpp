#pragma once

#include <string>

namespace mt {

class FilePath {
public:
  FilePath() = default;
  ~FilePath() = default;

  bool empty() const {
    return component.empty();
  }

private:
  std::string component;
};

}