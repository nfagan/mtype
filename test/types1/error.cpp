#include "error.hpp"
#include "type_representation.hpp"
#include "type_store.hpp"
#include "mt/text.hpp"
#include <cassert>

namespace mt {

void ShowUnificationErrors::show(const SimplificationFailure& err, std::string_view text, const CodeFileDescriptor& descriptor) const {
  const auto& at_token = *err.lhs_token;

  const bool is_null = at_token.type == TokenType::null;
  const auto start = is_null ? 0 : at_token.lexeme.data() - text.data();
  const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text.data();
  const int64_t context = 50;

  type_to_string.clear();
  type_to_string.apply(err.lhs_type);
  auto lhs_str = type_to_string.str();
  type_to_string.clear();
  type_to_string.apply(err.rhs_type);
  auto rhs_str = type_to_string.str();

  const std::string type_msg = lhs_str + " ≠ " + rhs_str + ".";
  auto msg = mark_text_with_message_and_context(text, start, stop, context, type_msg);

  std::cout << msg << std::endl;
}

void ShowUnificationErrors::show(const SimplificationFailures& errs, std::string_view text,
                                 const CodeFileDescriptor& descriptor) const {
  for (const auto& err : errs) {
    show(err, text, descriptor);
  }
}



}
