#pragma once

#include "token.hpp"
#include "utility.hpp"
#include <string>
#include <unordered_map>

namespace mt {

class CodeFileDescriptor;
class TextRowColumnIndices;

template <typename T>
class Optional;

class TokenSourceMap {
public:
  struct SourceData {
    SourceData() : SourceData(nullptr, nullptr, nullptr) {
      //
    }
    SourceData(const std::string* source, const CodeFileDescriptor* file_descriptor,
               const TextRowColumnIndices* row_col_indices) :
      source(source), file_descriptor(file_descriptor), row_col_indices(row_col_indices) {
      //
    }

    MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(SourceData)
    MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(SourceData)

    const std::string* source;
    const CodeFileDescriptor* file_descriptor;
    const TextRowColumnIndices* row_col_indices;
  };

public:
  TokenSourceMap() = default;
  ~TokenSourceMap() = default;

  void insert(const Token& tok, const SourceData& source_data);
  Optional<SourceData> lookup(const Token& tok) const;

private:
  std::unordered_map<Token, SourceData, Token::Hash> sources_by_token;
};

}