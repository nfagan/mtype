#include "text.hpp"
#include "character.hpp"
#include "string.hpp"
#include <cassert>

namespace mt {

void TextRowColumnIndices::scan(const char* text, int64_t size) {
  new_lines.clear();
  CharacterIterator it(text, size);

  while (it.has_next()) {
    const auto c = it.peek();

    if (c == Character('\n')) {
      new_lines.push_back(it.next_index());
    }

    it.advance();
  }
}

Optional<TextRowColumnIndices::Info> TextRowColumnIndices::line_info(int64_t index) const {
  using Info = TextRowColumnIndices::Info;

  if (index < 0 || new_lines.empty()) {
    return NullOpt{};
  }

  int64_t start = 0;
  int64_t end = new_lines.size()-1;

  while (start <= end) {
    const auto mid = (end - start) / 2 + start;
    const auto mid_val = new_lines[mid];

    if (mid_val == index) {
      return Optional<Info>(Info{mid + 1, 0});

    } else if (mid_val < index) {
      start = mid + 1;

    } else if (mid_val > index) {
      end = mid - 1;
    }
  }

  assert(start < int64_t(new_lines.size()));
  //  @TODO: Properly count utf8 characters between index and new line.
  return Optional<Info>(Info{start + 1, 0});
}

void swap(TextRowColumnIndices& a, TextRowColumnIndices& b) {
  using std::swap;
  swap(a.new_lines, b.new_lines);
}

std::string spaces(int num) {
  std::string spaces;
  for (int i = 0; i < num; i++) {
    spaces += " ";
  }
  return spaces;
}

std::string indent_spaces(const std::string& text, int num_spaces) {
  const auto spcs = spaces(num_spaces);
  auto split = split_copy(text.c_str(), text.size(), Character('\n'));
  for (auto& m : split) {
    m.insert(0, spcs);
  }
  return join(split, "\n");
}

MarkTextResult mark_text(std::string_view text, int64_t start, int64_t stop, int64_t context_amount) {
  //  @TODO: Robust handling of UTF8 context.
  int64_t begin_ind = std::max(int64_t(0), start - context_amount);
  int64_t stop_ind = std::min(int64_t(text.size()), stop + context_amount);
  int64_t len = stop_ind - begin_ind;

  int64_t subset_start_ind = start - begin_ind;
  std::string_view subset_text(text.data() + begin_ind, len);

  auto lines = split(subset_text, Character('\n'));
  auto new_line_inds = find_characters(subset_text.data(), subset_text.size(), Character('\n'));

  auto cumulative_inds = new_line_inds;
  cumulative_inds.insert(cumulative_inds.begin(), -1);
  new_line_inds.insert(new_line_inds.begin(), std::numeric_limits<int64_t>::min());

  uint64_t interval_ptr = 0;
  while (interval_ptr < new_line_inds.size() && subset_start_ind >= new_line_inds[interval_ptr]) {
    interval_ptr++;
  }

  if (interval_ptr == 0) {
    return MarkTextResult();
  }

  auto interval_ind = interval_ptr - 1;
  auto num_spaces = subset_start_ind - cumulative_inds[interval_ind] - 1;

  lines.erase(lines.begin() + interval_ind + 1, lines.end());

  return MarkTextResult(std::move(lines), num_spaces);
}

std::string mark_text_with_message_and_context(std::string_view text, int64_t start, int64_t stop,
                                               int64_t context_amount, const std::string& message) {
  auto mark_text_res = mark_text(text, start, stop, context_amount);
  if (!mark_text_res.success) {
    return "";
  }

  auto& lines = mark_text_res.lines;
  auto msg = spaces(mark_text_res.num_spaces_to_start);

  msg += "^\n";
  msg += message;

  lines.push_back(msg);
  return join(lines, "\n");
}

}
