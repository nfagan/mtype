#include "string.hpp"
#include "character.hpp"
#include <algorithm>
#include <limits>
#include <cassert>
#include <iomanip>
#include <sstream>

namespace mt {

int64_t StringRegistry::size() const {
  return strings.size();
}

std::string_view StringRegistry::at(int64_t index) const {
  assert(index >= 0 && index < int64_t(strings.size()) && "Out of bounds array access.");
  return strings[index];
}

std::vector<std::string_view> StringRegistry::collect(const std::vector<int64_t>& indices) const {
  std::vector<std::string_view> result;
  result.reserve(indices.size());
  for (const auto& index : indices) {
    result.push_back(at(index));
  }
  return result;
}

std::vector<std::string_view> StringRegistry::collect_n(const std::vector<int64_t>& indices, int64_t num) const {
  std::vector<std::string_view> result;
  result.reserve(num);
  for (int64_t i = 0; i < num; i++) {
    result.push_back(at(indices[i]));
  }
  return result;
}

int64_t StringRegistry::register_string(std::string_view str) {
#if MT_COPY_STRING_TO_REGISTRY
  auto search = std::string(str);
  auto it = string_registry.find(search);
#else
  auto it = string_registry.find(str);
#endif
  if (it == string_registry.end()) {
    //  String not yet registered.
    int64_t next_index = strings.size();
#if MT_COPY_STRING_TO_REGISTRY
    strings.push_back(search);
    string_registry[search] = next_index;
#else
    string_registry[str] = next_index;
    strings.push_back(str);
#endif
    return next_index;
  } else {
    return it->second;
  }
}

int64_t StringRegistry::make_registered_compound_identifier(const std::vector<int64_t>& components, int64_t num) {
  return register_string(join(collect_n(components, num), "."));
}

int64_t StringRegistry::make_registered_compound_identifier(const std::vector<int64_t>& components) {
  return make_registered_compound_identifier(components, components.size());
}

void StringRegistry::register_strings(const std::vector<std::string_view>& strs, std::vector<int64_t>& out) {
  for (const auto& str : strs) {
    out.push_back(register_string(str));
  }
}

std::vector<int64_t> StringRegistry::register_strings(const std::vector<std::string_view>& strs) {
  std::vector<int64_t> res;
  register_strings(strs, res);
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
