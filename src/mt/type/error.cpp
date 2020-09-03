#include "error.hpp"
#include "type_representation.hpp"
#include "../display.hpp"
#include "../text.hpp"
#include "../fs.hpp"
#include "mt/source_data.hpp"
#include "../config.hpp"
#include <cassert>

namespace mt {

BoxedTypeError make_recursive_type_error(const Token* at_token) {
  return std::make_unique<RecursiveTypeError>(at_token);
}

BoxedTypeError make_unresolved_function_error(const Token* at_token, const Type* function_type) {
  return std::make_unique<UnresolvedFunctionError>(at_token, function_type);
}

BoxedTypeError make_unknown_isa_guarded_class_error(const Token* at_token) {
  return std::make_unique<UnknownIsaGuardedClass>(at_token);
}

BoxedTypeError make_could_not_infer_type_error(const Token* at_token,
                                               std::string kind_str,
                                               const Type* in_type) {
  return std::make_unique<CouldNotInferTypeError>(at_token, std::move(kind_str), in_type);
}

BoxedTypeError make_bad_cast_error(const Token* at_token, const Type* cast) {
  return std::make_unique<BadCastError>(at_token, cast);
}

/*
 * SimplificationFailure
 */

std::string SimplificationFailure::get_text(const ShowTypeErrors& shower) const {
  const auto lhs_str = shower.type_to_string.apply(lhs_type);
  const auto rhs_str = shower.type_to_string.apply(rhs_type);

  if (shower.type_to_string.rich_text) {
#if MT_IS_MSVC
    return lhs_str + " /= " + rhs_str + ".";
#else
    return lhs_str + " ≠ " + rhs_str + ".";
#endif
  } else {
    return lhs_str + " /= " + rhs_str + ".";
  }
}

Token SimplificationFailure::get_source_token() const {
  return *lhs_token;
}

/*
 * OccursCheckFailure
 */

std::string OccursCheckFailure::get_text(const ShowTypeErrors& shower) const {
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

std::string UnresolvedFunctionError::get_text(const ShowTypeErrors& shower) const {
  const auto func_str = shower.type_to_string.apply(function_type);
  std::string msg = "No such function: ";
  std::string text = shower.stylize(style::red) + msg + shower.stylize(style::dflt);
  return text + func_str;
}

Token UnresolvedFunctionError::get_source_token() const {
  return *at_token;
}

/*
 * UnknownIsaGuardedClass
 */

Token UnknownIsaGuardedClass::get_source_token() const {
  return *at_token;
}

std::string UnknownIsaGuardedClass::get_text(const ShowTypeErrors& shower) const {
  std::string lexeme(at_token->lexeme);
  auto msg = "Class type for `" + lexeme + "` could not be located.";
  msg += "\n\nNote: if this class exists, it must be explicitly imported\nbefore the isa guard.";
  msg = shower.stylize(msg, style::red);
  return msg;
}

/*
 * InvalidFunctionInvocationError
 */

std::string InvalidFunctionInvocationError::get_text(const ShowTypeErrors& shower) const {
  const auto func_str = shower.type_to_string.apply(function_type);
  std::string msg = "Expected one `()` subscript for function type: ";
  std::string text = shower.stylize(style::red) + msg + shower.stylize(style::dflt);
  return text + func_str;
}

Token InvalidFunctionInvocationError::get_source_token() const {
  return *at_token;
}

/*
 * NonConstantFieldReferenceExprError
 */

std::string NonConstantFieldReferenceExprError::get_text(const ShowTypeErrors& shower) const {
  const auto func_str = shower.type_to_string.apply(arg_type);
  std::string msg = "Expected a constant expression for record field reference, but got: ";
  std::string text = shower.stylize(style::red) + msg + shower.stylize(style::dflt);
  return text + func_str;
}

Token NonConstantFieldReferenceExprError::get_source_token() const {
  return *at_token;
}

/*
 * NonexistentFieldReferenceError
 */

std::string NonexistentFieldReferenceError::get_text(const ShowTypeErrors& shower) const {
  const auto arg_str = shower.type_to_string.apply(arg_type);
  const auto field_str = shower.type_to_string.apply(field_type);
  const std::string field_msg = shower.stylize("Reference to non-existent field ", style::red) + field_str;
  const std::string of_msg = shower.stylize(" of ", style::red) + arg_str;
  return field_msg + of_msg;
}

Token NonexistentFieldReferenceError::get_source_token() const {
  return *at_token;
}

/*
 * UnhandledCustomSubscriptsError
 */

std::string UnhandledCustomSubscriptsError::get_text(const ShowTypeErrors& shower) const {
  auto type_str = shower.type_to_string.apply(arg_type);
  auto msg = shower.stylize("Custom subscripts are not yet handled for type: ") + type_str + ".";
  return msg;
}

Token UnhandledCustomSubscriptsError::get_source_token() const {
  return *at_token;
}

/*
 * DuplicateTypeIdentifierError
 */

std::string DuplicateTypeIdentifierError::get_text(const ShowTypeErrors&) const {
  return std::string("Duplicate type identifier `") + std::string(new_def->lexeme) + "`.";
}

Token DuplicateTypeIdentifierError::get_source_token() const {
  return *new_def;
}

/*
 * CouldNotInferTypeError
 */

std::string CouldNotInferTypeError::get_text(const ShowTypeErrors& shower) const {
  auto type_str = shower.type_to_string.apply(in_type);
  auto msg = "Could not infer " + kind_str + " in:\n";
  msg = shower.stylize(style::red) + msg + shower.stylize(style::dflt);
  msg += type_str + ".";
  return msg;
}

Token CouldNotInferTypeError::get_source_token() const {
  return *source_token;
}

/*
 * RecursiveTypeError
 */

std::string RecursiveTypeError::get_text(const ShowTypeErrors& shower) const {
  std::string msg = "Type directly or indirectly refers to itself.";
  return shower.stylize(msg, style::red);
}

Token RecursiveTypeError::get_source_token() const {
  return *source_token;
}

/*
 * BadCastError
 */

std::string BadCastError::get_text(const ShowTypeErrors& shower) const {
  auto cast_str = shower.type_to_string.apply(cast);
  const std::string msg = shower.stylize("Cannot cast: ", style::red);
  return msg + cast_str;
}

Token BadCastError::get_source_token() const {
  return *source_token;
}

/*
 * ShowUnificationErrors
 */

void ShowTypeErrors::show(const TypeError& err, int64_t index, const TokenSourceMap& source_data_by_token) const {
  const auto at_token = err.get_source_token();
  const auto maybe_source_data = source_data_by_token.lookup(at_token);
  assert(maybe_source_data);

  const auto& source_data = maybe_source_data.value();
  const auto& text_ptr = source_data.source.data();
  const auto& descriptor = *source_data.file_descriptor;
  const auto& row_col_indices = *source_data.row_col_indices;

  const bool is_null = at_token.is_null();
  const auto start = is_null ? 0 : at_token.lexeme.data() - text_ptr;
  const auto stop = is_null ? 0 : at_token.lexeme.data() + at_token.lexeme.size() - text_ptr;

  const std::string type_msg = err.get_text(*this);
  auto msg = mark_text_with_message_and_context(source_data.source, start, stop, context_amount, type_msg);
  msg = indent_spaces(msg, 2);

  std::cout << stylize(style::underline) << index;
  std::cout << ". " << descriptor.file_path << " ";

  auto new_line_res = row_col_indices.line_info(start);
  auto row = new_line_res ? new_line_res.value().row : -1;
  auto col = new_line_res ? new_line_res.value().column : -1;

  std::cout << "" << row << ":" << col << "";
  std::cout << stylize(style::dflt) << std::endl << std::endl;
  std::cout << msg << std::endl << std::endl << std::endl;
}

void ShowTypeErrors::show(const TypeErrors& errs, const TokenSourceMap& source_data) const {
  for (int64_t i = 0; i < int64_t(errs.size()); i++) {
    show(*errs[i], i+1, source_data);
  }
}

void ShowTypeErrors::show(const std::vector<const TypeError*> errs,
                          const TokenSourceMap& source_data) const {
  for (int64_t i = 0; i < int64_t(errs.size()); i++) {
    show(*errs[i], i+1, source_data);
  }
}

const char* ShowTypeErrors::stylize(const char* code) const {
  return type_to_string.rich_text ? code : "";
}

std::string ShowTypeErrors::stylize(const std::string& str, const char* style) const {
  return stylize(style) + str + stylize(style::dflt);
}

}
