#pragma once

#include "Result.hpp"
#include "Optional.hpp"
#include "token.hpp"
#include "character.hpp"
#include <string>
#include <vector>

namespace mt {

struct ScanError {
  std::string message;

  ScanError() = default;
  explicit ScanError(const char* message) : message(message) {
    //
  }
  explicit ScanError(std::string str) : message(std::move(str)) {
    //
  }

  ~ScanError() = default;
};

struct ScanErrors {
  std::vector<ScanError> errors;
};

using ScanResult = Result<ScanErrors, std::vector<Token>>;

class Scanner {
public:
  Scanner() = default;
  ~Scanner() = default;

  ScanResult scan(const char* text, int64_t len);
  ScanResult scan(const std::string& str);

private:
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
  void handle_punctuation(std::vector<Token>& tokens);

  bool is_within_type_annotation() const;

  Result<ScanError, Token> make_error_unterminated_string_literal(int64_t start);
  ScanError make_error_at_start(int64_t start, const char* message) const;

  static void check_add_token(Result<ScanError, Token>& res, ScanErrors& errs, std::vector<Token>& tokens);
private:
  CharacterIterator iterator;
  bool new_line_is_type_annotation_terminator;
  int block_comment_depth;
  int type_annotation_block_depth;
};

}