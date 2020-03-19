#include "string.hpp"
#include "character.hpp"
#include <algorithm>
#include <limits>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <array>

namespace mt {

int64_t StringRegistry::size() const {
  std::lock_guard<std::mutex> lock(mutex);
  return strings.size();
}

std::string StringRegistry::at(int64_t index) const {
  std::lock_guard<std::mutex> lock(mutex);
  assert(index >= 0 && index < int64_t(strings.size()) && "Out of bounds array access.");
  return strings[index];
}

std::vector<std::string> StringRegistry::collect(const std::vector<int64_t>& indices) const {
  std::vector<std::string> result;
  result.reserve(indices.size());
  for (const auto& index : indices) {
    result.emplace_back(at(index));
  }
  return result;
}

int64_t StringRegistry::register_string(std::string_view str) {
  std::lock_guard<std::mutex> lock(mutex);

  auto search = std::string(str);
  auto it = string_registry.find(search);

  if (it == string_registry.end()) {
    //  String not yet registered.
    const int64_t next_index = strings.size();
    strings.push_back(search);
    string_registry[search] = next_index;

    return next_index;
  } else {
    return it->second;
  }
}

std::string StringRegistry::make_compound_identifier(const std::vector<int64_t>& components) const {
  return join(collect(components), ".");
}

int64_t StringRegistry::make_registered_compound_identifier(const std::vector<int64_t>& components) {
  return register_string(join(collect(components), "."));
}

std::vector<int64_t> StringRegistry::register_strings(const std::vector<std::string_view>& strs) {
  std::vector<int64_t> res;
  res.reserve(strs.size());

  for (const auto& str : strs) {
    res.emplace_back(register_string(str));
  }

  return res;
}

namespace {
template <typename T>
std::vector<T> split(const char* str, int64_t len, const Character& delim) {
  CharacterIterator it(str, len);
  std::vector<T> result;

  int64_t offset = 0;
  int64_t index = 0;

  while (it.has_next()) {
    const auto& c = it.peek();

    if (c == delim) {
      T slice(str + offset, index - offset);
      result.emplace_back(slice);
      offset = index + c.count_units();
    }

    it.advance();
    index += c.count_units();
  }

  result.emplace_back(T(str + offset, len - offset));

  return result;
}

template <typename T>
std::vector<T> split_whitespace(const char* str, int64_t len, bool preserve_whitespace) {
  CharacterIterator it(str, len);
  std::vector<T> result;

  int64_t offset = 0;
  int64_t index = 0;

  const std::array<Character, 3> whitespaces{{Character('\n'), Character(' '), Character('\t')}};

  while (it.has_next()) {
    const auto& c = it.peek();
    const auto num_units = c.count_units();

    for (const auto& delim : whitespaces) {
      if (c == delim) {
        const auto addtl_offset = preserve_whitespace ? num_units : 0;

        T slice(str + offset, index - offset + addtl_offset);
        result.emplace_back(slice);
        offset = index + num_units;
        break;
      }
    }

    it.advance();
    index += num_units;
  }

  result.emplace_back(T(str + offset, len - offset));

  return result;
}

}

std::vector<std::string_view> split(std::string_view view, const Character& delim) {
  return split(view.data(), view.size(), delim);
}

std::vector<std::string_view> split(const std::string& str, const Character& delim) {
  return split(str.c_str(), str.size(), delim);
}

std::vector<std::string_view> split(const char* str, int64_t len, const Character& delim) {
  return split<std::string_view>(str, len, delim);
}

std::vector<std::string> split_copy(const char *str, int64_t len, const Character& delim) {
  return split<std::string>(str, len, delim);
}

std::vector<std::string> split_whitespace_copy(const char* str, int64_t len, bool preserve) {
  return split_whitespace<std::string>(str, len, preserve);
}

std::vector<int64_t> find_character(const char* str, int64_t len, const Character& look_for) {
  CharacterIterator it(str, len);
  std::vector<int64_t> result;

  while (it.has_next()) {
    auto c = it.peek();

    if (c == look_for) {
      result.push_back(it.next_index());
    }

    it.advance();
  }

  return result;
}

std::string ptr_to_hex_string(const void* ptr) {
  std::stringstream stream;
  stream << std::hex << uint64_t(ptr);
  return stream.str();
}

}
