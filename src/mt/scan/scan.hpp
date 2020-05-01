#pragma once

#include "../Result.hpp"
#include "../Optional.hpp"
#include "../token.hpp"
#include "../character.hpp"
#include "../text.hpp"
#include "../utility.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace mt {

struct ScanError {
  ScanError() = default;
  explicit ScanError(const char* message) : message(message) {
    //
  }
  explicit ScanError(std::string str) : message(std::move(str)) {
    //
  }

  std::string message;
  static constexpr int context_amount = 30;
};

struct EndTerminatedKeywordCounts {
  EndTerminatedKeywordCounts() = default;
  ~EndTerminatedKeywordCounts() = default;

  bool parent_is_classdef() const;
  bool is_non_end_terminated_function_file() const;

  Optional<ScanError> register_keyword(TokenType keyword_type,
                                       bool requires_end,
                                       const CharacterIterator& iterator,
                                       int64_t start);
  Optional<ScanError> pop_keyword(const CharacterIterator& iterator, int64_t start);

  std::vector<TokenType> keyword_types;
  std::unordered_map<TokenType, int64_t> keyword_counts;
};

using ScanErrors = std::vector<ScanError>;

struct ScanInfo {
  ScanInfo() : functions_are_end_terminated(true) {
    //
  }
  ScanInfo(std::vector<Token>&& tokens, TextRowColumnIndices&& inds) :
  tokens(std::move(tokens)),
  functions_are_end_terminated(true),
  row_column_indices(std::move(inds)) {
    //
  }
  ~ScanInfo() = default;

  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(ScanInfo)

  std::vector<Token> tokens;
  bool functions_are_end_terminated;
  TextRowColumnIndices row_column_indices;
};

//  swap for ScanInfo
void swap(ScanInfo& lhs, ScanInfo& rhs);

using ScanResult = Result<ScanErrors, ScanInfo>;

class Scanner {
public:
  Scanner() = default;
  ~Scanner() = default;

  ScanResult scan(const char* text, int64_t len);
  ScanResult scan(const std::string& str);

private:
  void begin_scan(const char* text, int64_t len);
  ScanInfo finalize_scan(std::vector<Token>& tokens) const;

  std::string_view make_lexeme(int64_t offset, int64_t len) const;
  void consume_whitespace_to_new_line();
  void consume_whitespace();
  void consume_to_new_line();

  Result<ScanError, Token> identifier_or_keyword_token();
  Token number_literal_token();
  Result<ScanError, Token> string_literal_token(TokenType type, const Character& terminator);
  Optional<ScanError> handle_comment(std::vector<Token>& tokens);
  Optional<ScanError> handle_type_annotation_initializer(std::vector<Token>& tokens, bool is_block_comment);
  void handle_new_line(std::vector<Token>& tokens);
  Optional<ScanError> handle_punctuation(std::vector<Token>& tokens);
  Optional<ScanError> update_grouping_character_depth(const Character& c, int64_t start);

  bool is_within_type_annotation() const;
  bool is_scientific_notation_number_literal(int* num_to_advance) const;

  Result<ScanError, Token> make_error_unterminated_string_literal(int64_t start) const;
  ScanError make_error_unbalanced_grouping_character(int64_t start) const;
  ScanError make_error_at_start(int64_t start, const char* message) const;

  static void check_add_token(Result<ScanError, Token>& res, ScanErrors& errs, std::vector<Token>& tokens);
private:
  CharacterIterator iterator;
  bool new_line_is_type_annotation_terminator;
  int block_comment_depth;
  int type_annotation_block_depth;
  bool marked_unbalanced_grouping_character_error;

  int parens_depth;
  int brace_depth;
  int bracket_depth;

  EndTerminatedKeywordCounts keyword_counts;

  std::string_view source_text;
};

}