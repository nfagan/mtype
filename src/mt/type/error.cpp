#include "error.hpp"
#include "type_representation.hpp"
#include "../display.hpp"
#include "../text.hpp"
#include "../fs.hpp"
#include <cassert>

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

void ShowUnificationErrors::show(const UnificationError& err, int64_t index, const TokenSourceMap& source_data_by_token) const {
  const auto at_token = err.get_source_token();
  const auto maybe_source_data = source_data_by_token.lookup(at_token);
  assert(maybe_source_data);

  const auto& source_data = maybe_source_data.value();
  const auto& text_ptr = source_data.source->data();
  const auto& descriptor = *source_data.file_descriptor;
  const auto& row_col_indices = *source_data.row_col_indices;

  const bool is_null = at_token.is_null();
  const auto start = is_null ? 0 : at_token.lexeme.data() - text_ptr;
  const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text_ptr;

  const std::string type_msg = err.get_text(*this);
  auto msg = mark_text_with_message_and_context(*source_data.source, start, stop, context_amount, type_msg);
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

void ShowUnificationErrors::show(const UnificationErrors& errs, const TokenSourceMap& source_data) const {

  for (int64_t i = 0; i < int64_t(errs.size()); i++) {
    show(*errs[i], i+1, source_data);
  }
}

const char* ShowUnificationErrors::stylize(const char* code) const {
  return rich_text ? code : "";
}

/*
 * TokenSourceMap
 */

void TokenSourceMap::insert(const Token& tok, const TokenSourceMap::SourceData& data) {
  sources_by_token[tok] = data;
}

Optional<TokenSourceMap::SourceData> TokenSourceMap::lookup(const Token& tok) const {
  auto it = sources_by_token.find(tok);
  return it == sources_by_token.end() ? NullOpt{} : Optional<TokenSourceMap::SourceData>(it->second);
}

}
