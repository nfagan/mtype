#pragma once

#include "type.hpp"
#include <string>
#include <vector>

namespace mt {

struct Token;
class TypeStore;
class TypeToString;
class ShowUnificationErrors;

struct UnificationError {
  virtual std::string get_text(const ShowUnificationErrors& shower) const = 0;
  virtual Token get_source_token() const = 0;
  virtual ~UnificationError() = default;
};

struct SimplificationFailure : public UnificationError {
  SimplificationFailure(const Token* lhs_token, const Token* rhs_token,
                        const Type* lhs_type, const Type* rhs_type) :
  lhs_token(lhs_token), rhs_token(rhs_token), lhs_type(lhs_type), rhs_type(rhs_type) {
    //
  }
  ~SimplificationFailure() override = default;

  std::string get_text(const ShowUnificationErrors& shower) const override;
  Token get_source_token() const override;

  const Token* lhs_token;
  const Token* rhs_token;
  const Type* lhs_type;
  const Type* rhs_type;
};

struct OccursCheckFailure : public UnificationError {
  OccursCheckFailure(const Token* lhs_token, const Token* rhs_token,
                     const Type* lhs_type, const Type* rhs_type) :
  lhs_token(lhs_token), rhs_token(rhs_token), lhs_type(lhs_type), rhs_type(rhs_type) {
    //
  }
  ~OccursCheckFailure() override = default;

  std::string get_text(const ShowUnificationErrors& shower) const override;
  Token get_source_token() const override;

  const Token* lhs_token;
  const Token* rhs_token;
  const Type* lhs_type;
  const Type* rhs_type;
};

struct UnresolvedFunctionError : public UnificationError {
  UnresolvedFunctionError(const Token* at_token, const Type* function_type) :
  at_token(at_token), function_type(function_type) {
    //
  }
  ~UnresolvedFunctionError() override = default;
  std::string get_text(const ShowUnificationErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* function_type;
};

struct InvalidFunctionInvocationError : public UnificationError {
  InvalidFunctionInvocationError(const Token* at_token, const Type* function_type) :
  at_token(at_token), function_type(function_type) {
    //
  }
  ~InvalidFunctionInvocationError() override = default;
  std::string get_text(const ShowUnificationErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* function_type;
};

using BoxedUnificationError = std::unique_ptr<UnificationError>;
using UnificationErrors = std::vector<BoxedUnificationError>;

class ShowUnificationErrors {
public:
  ShowUnificationErrors(const TypeStore& store, const TypeToString& type_to_string) :
  store(store), type_to_string(type_to_string), rich_text(true) {
    //
  }

  void show(const UnificationError& err, int64_t index, std::string_view text,
            const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const;
  void show(const UnificationErrors& errs, std::string_view text,
    const CodeFileDescriptor& descriptor, const TextRowColumnIndices& row_col_indices) const;

  const char* stylize(const char* code) const;

private:
  const TypeStore& store;
  static constexpr int context_amount = 50;

public:
  const TypeToString& type_to_string;
  bool rich_text;
};

}