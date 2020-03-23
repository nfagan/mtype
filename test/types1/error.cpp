#include "error.hpp"
#include "type_representation.hpp"
#include "type_store.hpp"
#include "mt/text.hpp"
#include "mt/display.hpp"
#include <cassert>

namespace mt {

void ShowUnificationErrors::show(const SimplificationFailure& err, int64_t index, std::string_view text,
  const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const {
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
  msg = indent_spaces(msg, 2);

  std::cout << stylize(style::bold) << index;
  std::cout << ". " << descriptor.file_path << " ";

  auto new_line_res = row_col_indices.line_info(start);
  auto row = new_line_res ? new_line_res.value().row : -1;
  auto col = new_line_res ? new_line_res.value().column : -1;

  std::cout << "" << row << ":" << col << "";
  std::cout << stylize(style::dflt) << std::endl << std::endl;
  std::cout << msg << std::endl << std::endl;
}

void ShowUnificationErrors::show(const SimplificationFailures& errs, std::string_view text,
  const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const {

  for (int64_t i = 0; i < errs.size(); i++) {
    show(errs[i], i+1, text, descriptor, row_col_indices);
  }
}

const char* ShowUnificationErrors::stylize(const char* code) const {
  return rich_text ? code : "";
}

}
