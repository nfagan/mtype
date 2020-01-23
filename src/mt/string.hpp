#pragma once

#include <vector>
#include <string_view>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace mt {
class Character;

class StringRegistry {
public:
  StringRegistry() = default;
  ~StringRegistry() = default;

  int64_t register_string(std::string_view str);
  void register_strings(const std::vector<std::string_view>& strs, std::vector<int64_t>& out);
  std::vector<int64_t> register_strings(const std::vector<std::string_view>& strs);

  std::string_view at(int64_t index) const;
  std::vector<std::string_view> collect(const std::vector<int64_t>& indices) const;

  int64_t size() const;

private:
  std::unordered_map<std::string_view, int64_t> string_registry;
  std::vector<std::string_view> strings;
};

std::vector<std::string_view> split(const char* str, int64_t len, const Character& delim);
std::vector<std::string_view> split(const std::string& str, const Character& delim);
std::vector<std::string_view> split(std::string_view view, const Character& delim);

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