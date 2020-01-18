#pragma once

#include <vector>
#include <string_view>
#include <string>

namespace mt {
  class Character;

  std::vector<std::string_view> split(const char* str, int64_t len, const Character& delim);
  std::vector<std::string_view> split(const std::string& str, const Character& delim);
  std::vector<std::string_view> split(std::string_view view, const Character& delim);

  std::string join(const std::vector<std::string_view>& strs, const Character& by);

  std::string mark_text_with_message_and_context(std::string_view text, int64_t start, int64_t stop,
                                                 int64_t context_amount, const std::string& message);

  std::vector<int64_t> find_character(const char* str, int64_t len, const Character& delim);
}