#pragma once

#include "type.hpp"
#include <string>
#include <vector>

namespace mt {

struct Token;
class TypeStore;
class TypeToString;

struct SimplificationFailure {
  SimplificationFailure(const Token* lhs_token, const Token* rhs_token, TypeRef lhs_type, TypeRef rhs_type) :
  lhs_token(lhs_token), rhs_token(rhs_token), lhs_type(lhs_type), rhs_type(rhs_type) {
    //
  }

  const Token* lhs_token;
  const Token* rhs_token;
  TypeHandle lhs_type;
  TypeHandle rhs_type;
};

using SimplificationFailures = std::vector<SimplificationFailure>;

class ShowUnificationErrors {
public:
  ShowUnificationErrors(const TypeStore& store, TypeToString& type_to_string) :
  store(store), type_to_string(type_to_string), rich_text(true) {
    //
  }

  void show(const SimplificationFailure& err, int64_t index, std::string_view text,
    const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const;
  void show(const SimplificationFailures& errs, std::string_view text,
    const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const;

private:
  const char* stylize(const char* code) const;

private:
  const TypeStore& store;
  TypeToString& type_to_string;

public:
  bool rich_text;
};

}