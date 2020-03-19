#pragma once

#include "Optional.hpp"
#include "utility.hpp"
#include <cstdint>
#include <vector>

namespace mt {

class TextRowColumnIndices {
  struct Info {
    int64_t row;
    int64_t column;
  };

public:
  TextRowColumnIndices() = default;

  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(TextRowColumnIndices)

  void scan(const char* text, int64_t size);
  Optional<Info> line_info(int64_t index) const;

  friend void swap(TextRowColumnIndices& a, TextRowColumnIndices& b);

private:
  std::vector<int64_t> new_lines;
};

}