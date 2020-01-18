#pragma once

#include <vector>
#include <string_view>
#include <string>

namespace mt {
  class Character;

  std::vector<std::string_view> split(const char* str, int64_t len, const Character& delim);
  std::vector<std::string_view> split(const std::string& str, const Character& delim);
  std::vector<std::string_view> split(std::string_view view, const Character& delim);

  template <typename T>
  std::string join(const std::vector<T>& strs, const Character& by) {
    std::string result;
    const int64_t n = strs.size();

    for (int64_t i = 0; i < n; i++) {
      result += strs[i];

      if (i < n-1) {
        result += T(by);
      }
    }

    return result;
  }

  template <typename T>
  std::string join(const std::vector<T>& strs, const std::string& by) {
    std::string result;
    const int64_t n = strs.size();

    for (int64_t i = 0; i < n; i++) {
      result += strs[i];

      if (i < n-1) {
        result += by;
      }
    }

    return result;
  }

  std::string mark_text_with_message_and_context(std::string_view text, int64_t start, int64_t stop,
                                                 int64_t context_amount, const std::string& message);

  std::vector<int64_t> find_character(const char* str, int64_t len, const Character& delim);
}