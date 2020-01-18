#pragma once

#include "Result.hpp"
#include "token.hpp"
#include "character.hpp"
#include <string>
#include <vector>

namespace mt {

struct ScanError {
  std::string message;
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

  Token identifier_or_keyword_token();
  Token number_literal_token();
  Result<ScanError, Token> string_literal_token(TokenType type, const Character& terminator);

  Result<ScanError, Token> make_error_unterminated_string_literal(int64_t start);

  static void check_add_token(Result<ScanError, Token>& res, ScanErrors& errs, std::vector<Token>& tokens);
private:
  CharacterIterator iterator;
};

}