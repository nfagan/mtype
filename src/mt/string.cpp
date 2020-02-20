#include "string.hpp"
#include "character.hpp"
#include <algorithm>
#include <limits>
#include <cassert>
#include <iomanip>
#include <sstream>

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

std::vector<std::string_view> split(std::string_view view, const Character& delim) {
  return split(view.data(), view.size(), delim);
}

std::vector<std::string_view> split(const std::string& str, const Character& delim) {
  return split(str.c_str(), str.size(), delim);
}

std::vector<std::string_view> split(const char* str, int64_t len, const Character& delim) {
  CharacterIterator it(str, len);
  std::vector<std::string_view> result;

  int64_t offset = 0;
  int64_t index = 0;

  while (it.has_next()) {
    const auto& c = it.peek();

    if (c == delim) {
      std::string_view view(str + offset, index - offset);
      result.emplace_back(view);
      offset = index + c.count_units();
    }

    it.advance();
    index += c.count_units();
  }

  result.emplace_back(std::string_view(str + offset, len - offset));

  return result;
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

std::string mark_text_with_message_and_context(std::string_view text, int64_t start, int64_t stop,
                                               int64_t context_amount, const std::string& message) {
  //  @TODO: Robust handling of UTF8 context.
  int64_t begin_ind = std::max(int64_t(0), start - context_amount);
  int64_t stop_ind = std::min(int64_t(text.size()), stop + context_amount);
  int64_t len = stop_ind - begin_ind;

  int64_t subset_start_ind = start - begin_ind;
  std::string_view subset_text(text.data() + begin_ind, len);

  auto lines = split(subset_text, Character('\n'));
  auto new_line_inds = find_character(subset_text.data(), subset_text.size(), Character('\n'));

  auto cumulative_inds = new_line_inds;
  cumulative_inds.insert(cumulative_inds.begin(), -1);
  new_line_inds.insert(new_line_inds.begin(), std::numeric_limits<int64_t>::min());

  uint64_t interval_ptr = 0;
  while (interval_ptr < new_line_inds.size() && subset_start_ind >= new_line_inds[interval_ptr]) {
    interval_ptr++;
  }

  if (interval_ptr == 0) {
    return "";
  }

  auto interval_ind = interval_ptr - 1;
  auto num_spaces = subset_start_ind - cumulative_inds[interval_ind] - 1;

  std::string result;

  for (int i = 0; i < num_spaces; i++) {
    result += " ";
  }

  result += "^\n";
  result += message;

  lines.erase(lines.begin() + interval_ind + 1, lines.end());
  lines.push_back(result);

  return join(lines, "\n");
}

}
