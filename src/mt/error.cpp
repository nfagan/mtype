#include "error.hpp"
#include "string.hpp"

namespace mt {

void ParseError::show() const {
  if (!message.empty()) {
    const bool is_null = at_token.type == TokenType::null;
    const auto start = is_null ? 0 : at_token.lexeme.data() - text.data();
    const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text.data();

    auto msg = mark_text_with_message_and_context(text, start, stop, 50, message);
    std::cout << msg << std::endl;
  }
}

}
