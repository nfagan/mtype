#pragma once

#include <vector>
#include <string_view>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <mutex>

namespace mt {
class Character;
template <typename T>
class Optional;

class StringRegistry {
public:
  StringRegistry() = default;
  ~StringRegistry() = default;

  int64_t register_string(std::string_view str);
  std::vector<int64_t> register_strings(const std::vector<std::string_view>& strs);

  int64_t make_registered_compound_identifier(const std::vector<int64_t>& components);
  std::string make_compound_identifier(const std::vector<int64_t>& components) const;

  bool contains(int64_t id) const;
  std::string at(int64_t index) const;
  Optional<std::string> maybe_at(int64_t index) const;

  std::vector<std::string> collect(const std::vector<int64_t>& indices) const;

  int64_t size() const;

private:
  mutable std::mutex mutex;

  std::unordered_map<std::string, int64_t> string_registry;
  std::vector<std::string> strings;
};

std::vector<std::string_view> split(const char* str, int64_t len, const Character& delim);
std::vector<std::string_view> split(const std::string& str, const Character& delim);
std::vector<std::string_view> split(std::string_view view, const Character& delim);

std::vector<std::string> split_copy(const char* str, int64_t len, const Character& delim);
std::vector<std::string> split_whitespace_copy(const char* str, int64_t len, bool preserve = false);

bool matches_view(std::string_view str, const char* str2);

std::string utf8_reverse(const std::string& str);

std::string ptr_to_hex_string(const void* ptr);

template <typename T>
std::string join(const std::vector<T>& strs, const std::string& by, int64_t n);

template <typename T>
std::string join(const std::vector<T>& strs, const std::string& by);

std::vector<int64_t> find_characters(const char* str, int64_t len, const Character& delim);
int64_t find_character(const char* str, int64_t len, const Character& search);

}

/*
 * Impl
 */

template <typename T>
std::string mt::join(const std::vector<T>& strs, const std::string& by, int64_t n) {
  std::string result;

  for (int64_t i = 0; i < n; i++) {
    result += strs[i];

    if (i < n-1) {
      result += by;
    }
  }

  return result;
}

template <typename T>
std::string mt::join(const std::vector<T>& strs, const std::string& by) {
  return join(strs, by, strs.size());
}