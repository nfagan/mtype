#include "error.hpp"
#include "string.hpp"
#include "character.hpp"
#include "text.hpp"
#include "display.hpp"

namespace mt {

namespace {
inline std::string transform_message(const std::string& msg) {
  auto split_msg = split_copy(msg.c_str(), msg.size(), Character('\n'));;
  for (auto& m : split_msg) {
    m = "  " + m;
  }
  return join(split_msg, "\n");
}
}

bool ParseError::is_null_token() const {
  return at_token.type == TokenType::null;
}

std::string ParseError::make_message(bool colorize) const {
  const bool is_null = is_null_token();
  const auto start = is_null ? 0 : at_token.lexeme.data() - text.data();
  const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text.data();
  const int64_t context = 50;

  std::string msg;

  if (colorize) {
    const auto tmp = style::red + message + style::dflt;
    msg = mark_text_with_message_and_context(text, start, stop, context, tmp);
  } else {
    msg = mark_text_with_message_and_context(text, start, stop, context, message);
  }

  return transform_message(msg);
}

void ParseError::show(int64_t index) const {
  if (!message.empty()) {
    const auto transformed = make_message(false);

    std::cout << index << "." << std::endl << std::endl;
    std::cout << transformed << std::endl << std::endl;
  }
}

void ParseError::show(const TextRowColumnIndices& row_col_indices, int64_t index) const {
  if (!message.empty()) {
    const auto transformed = make_message(true);
    const auto start = is_null_token() ? 0 : at_token.lexeme.data() - text.data();

    auto new_line_res = row_col_indices.line_info(start);
    auto row = new_line_res ? new_line_res.value().row : -1;
    auto col = new_line_res ? new_line_res.value().column : -1;

    std::cout << index << ". @ ";
    std::cout << row << ":" << col << "." << std::endl << std::endl;
    std::cout << transformed << std::endl << std::endl;
  }
}

void show_parse_errors(const ParseErrors& errs, const TextRowColumnIndices& row_col_inds) {
  int64_t index = 0;
  for (const auto& err : errs) {
    err.show(row_col_inds, ++index);
  }
}

}
