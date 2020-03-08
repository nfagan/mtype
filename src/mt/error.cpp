#include "error.hpp"
#include "string.hpp"
#include "character.hpp"

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

void ParseError::show(int64_t index) const {
  if (!message.empty()) {
    const bool is_null = at_token.type == TokenType::null;
    const auto start = is_null ? 0 : at_token.lexeme.data() - text.data();
    const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text.data();

    auto msg = mark_text_with_message_and_context(text, start, stop, 50, message);
    auto transformed = transform_message(msg);

    std::cout << index << "." << std::endl << std::endl;
    std::cout << transformed << std::endl << std::endl;
//    std::cout << msg << std::endl << std::endl;
  }
}

void show_parse_errors(const ParseErrors& errs) {
  int64_t index = 0;
  for (const auto& err : errs) {
    err.show(++index);
  }
}

}
