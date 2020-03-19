#pragma once

#include "Optional.hpp"
#include "utility.hpp"
#include <cstdint>
#include <vector>
#include <string_view>

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

std::string mark_text_with_message_and_context(std::string_view text, int64_t start, int64_t stop,
                                               int64_t context_amount, const std::string& message);

}