#pragma once

#include "token.hpp"
#include "utility.hpp"
#include <string>
#include <unordered_map>
#include <string_view>

namespace mt {

class CodeFileDescriptor;
class TextRowColumnIndices;

struct ParseSourceData {
  ParseSourceData() : ParseSourceData(std::string_view(), nullptr, nullptr) {
    //
  }
  ParseSourceData(std::string_view source, const CodeFileDescriptor* file_descriptor,
                  const TextRowColumnIndices* row_col_indices) :
    source(source), file_descriptor(file_descriptor), row_col_indices(row_col_indices) {
    //
  }

  MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(ParseSourceData)
  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(ParseSourceData)

  std::string_view source;
  const CodeFileDescriptor* file_descriptor;
  const TextRowColumnIndices* row_col_indices;
};

template <typename T>
class Optional;

class TokenSourceMap {
public:
  TokenSourceMap() = default;
  ~TokenSourceMap() = default;

  void insert(const Token& tok, const ParseSourceData& source_data);
  Optional<ParseSourceData> lookup(const Token& tok) const;

private:
  std::unordered_map<Token, ParseSourceData, Token::Hash> sources_by_token;
};

}