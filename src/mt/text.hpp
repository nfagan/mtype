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

struct MarkTextResult {
  MarkTextResult() : num_spaces_to_start(-1), success(false) {
    //
  }

  MarkTextResult(std::vector<std::string_view>&& lines, int num_spaces_to_start) :
  lines(std::move(lines)), num_spaces_to_start(num_spaces_to_start), success(true) {
    //
  }

  std::vector<std::string_view> lines;
  int num_spaces_to_start;
  bool success;
};

MarkTextResult mark_text(std::string_view text, int64_t start, int64_t stop, int64_t context_amount);

std::string mark_text_with_message_and_context(std::string_view text, int64_t start, int64_t stop,
                                               int64_t context_amount, const std::string& message);

std::string indent_spaces(const std::string& text, int num_spaces);
std::string spaces(int num);

}