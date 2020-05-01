#pragma once

#include "fs/path.hpp"
#include <string>
#include <unordered_set>

namespace mt {

class ErrorFilter {
public:
  bool matches_identifier(const FilePath& file_path) const;

public:
  std::unordered_set<std::string> identifiers;
};

}