#include "error.hpp"
#include "type_representation.hpp"
#include "type_store.hpp"
#include "mt/text.hpp"
#include "mt/display.hpp"

namespace mt {

/*
 * SimplificationFailure
 */

std::string SimplificationFailure::get_text(const ShowUnificationErrors& shower) const {
  const auto lhs_str = shower.type_to_string.apply(lhs_type);
  const auto rhs_str = shower.type_to_string.apply(rhs_type);

  return lhs_str + " ≠ " + rhs_str + ".";
}

Token SimplificationFailure::get_source_token() const {
  return *lhs_token;
}

/*
 * OccursCheckFailure
 */

std::string OccursCheckFailure::get_text(const ShowUnificationErrors& shower) const {
  const auto lhs_str = shower.type_to_string.apply(lhs_type);
  const auto rhs_str = shower.type_to_string.apply(rhs_type);
  const std::string msg = "No occurrence violation: ";
  std::string text = shower.stylize(style::red) + msg + shower.stylize(style::dflt);
  return text + lhs_str + " = " + rhs_str + ".";
}

Token OccursCheckFailure::get_source_token() const {
  return *lhs_token;
}

/*
 * UnresolvedFunctionError
 */

std::string UnresolvedFunctionError::get_text(const ShowUnificationErrors& shower) const {
  const auto func_str = shower.type_to_string.apply(function_type);
  std::string msg = "No such function: ";
  std::string text = shower.stylize(style::red) + msg + shower.stylize(style::dflt);
  return text + func_str;
}

Token UnresolvedFunctionError::get_source_token() const {
  return *at_token;
}

/*
 * InvalidFunctionInvocationError
 */

std::string InvalidFunctionInvocationError::get_text(const ShowUnificationErrors& shower) const {
  const auto func_str = shower.type_to_string.apply(function_type);
  std::string msg = "Expected one `()` subscript for function type: ";
  std::string text = shower.stylize(style::red) + msg + shower.stylize(style::dflt);
  return text + func_str;
}

Token InvalidFunctionInvocationError::get_source_token() const {
  return *at_token;
}

/*
 * ShowUnificationErrors
 */

void ShowUnificationErrors::show(const UnificationError& err, int64_t index, std::string_view text,
                                 const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const {
  const auto at_token = err.get_source_token();

  const bool is_null = at_token.is_null();
  const auto start = is_null ? 0 : at_token.lexeme.data() - text.data();
  const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text.data();

  const std::string type_msg = err.get_text(*this);
  auto msg = mark_text_with_message_and_context(text, start, stop, context_amount, type_msg);
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

void ShowUnificationErrors::show(const UnificationErrors& errs, std::string_view text,
  const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const {

  for (int64_t i = 0; i < int64_t(errs.size()); i++) {
    show(*errs[i], i+1, text, descriptor, row_col_indices);
  }
}

const char* ShowUnificationErrors::stylize(const char* code) const {
  return rich_text ? code : "";
}

}
