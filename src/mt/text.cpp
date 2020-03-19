#include "text.hpp"
#include "character.hpp"
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

}
