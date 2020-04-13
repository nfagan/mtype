#include "error.hpp"
#include "string.hpp"
#include "character.hpp"
#include "text.hpp"
#include "display.hpp"
#include "keyword.hpp"
#include "fs/code_file.hpp"
#include "source_data.hpp"
#include <cassert>

namespace mt {

namespace {
inline std::string colorize_message(const std::string& msg) {
  auto split_msg = split_whitespace_copy(msg.c_str(), msg.size(), true);

  for (auto& m : split_msg) {
    if (matlab::begins_with_keyword(m)) {
      m.insert(0, style::yellow);
      m.append(style::dflt);
    }
  }

  return join(split_msg, "");
}
}

bool ParseError::is_null_token() const {
  return at_token.is_null();
}

std::string ParseError::make_message(bool colorize) const {
  const bool is_null = is_null_token();
  const auto start = is_null ? 0 : at_token.lexeme.data() - text.data();
  const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text.data();

  std::string msg;

  if (colorize) {
    const auto tmp = style::red + message + style::dflt;
    msg = mark_text_with_message_and_context(text, start, stop, context_amount, tmp);
    msg = colorize_message(msg);

  } else {
    msg = mark_text_with_message_and_context(text, start, stop, context_amount, message);
  }

  return indent_spaces(msg, 2);
}

void ShowParseErrors::show(const ParseErrors& errs, const TokenSourceMap& sources_by_token) {
  for (int64_t i = 0; i < int64_t(errs.size()); i++) {
    show(errs[i], sources_by_token, i+1);
  }
}

void ShowParseErrors::show(const ParseError& err, const TokenSourceMap& sources_by_token, int64_t index) {
  if (!err.message.empty()) {
    const auto& maybe_source_data = sources_by_token.lookup(err.at_token);
    assert(maybe_source_data);
    const auto& source_data = maybe_source_data.value();
    const auto& row_col_indices = source_data.row_col_indices;

    const auto transformed = err.make_message(is_rich_text);
    const auto start = err.is_null_token() ? 0 : err.at_token.lexeme.data() - err.text.data();

    std::cout << stylize(style::bold) << index;

    if (err.descriptor) {
      std::cout << ". " << err.descriptor->file_path << " ";
    } else {
      std::cout << ". <anonymous> ";
    }

    if (row_col_indices) {
      auto new_line_res = row_col_indices->line_info(start);
      auto row = new_line_res ? new_line_res.value().row : -1;
      auto col = new_line_res ? new_line_res.value().column : -1;

      std::cout << "" << row << ":" << col << "";
    }

    std::cout << stylize(style::dflt) << std::endl << std::endl;
    std::cout << transformed << std::endl << std::endl;
  }
}

const char* ShowParseErrors::stylize(const char* code) const {
  return is_rich_text ? code : "";
}

/*
 * Util
 */

std::string make_error_message_duplicate_type_identifier(std::string_view dup_ident) {
  return "Duplicate type identifier `" + std::string(dup_ident) + "`.";
}

}
