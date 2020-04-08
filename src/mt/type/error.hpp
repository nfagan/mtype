#pragma once

#include "type.hpp"
#include "../Optional.hpp"
#include "../token.hpp"
#include <string>
#include <vector>

namespace mt {

class TypeStore;
class TypeToString;
class ShowUnificationErrors;
class CodeFileDescriptor;
class TextRowColumnIndices;

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

struct NonConstantFieldReferenceExprError : public UnificationError {
  NonConstantFieldReferenceExprError(const Token* at_token, const Type* arg_type) :
  at_token(at_token), arg_type(arg_type) {
    //
  }
  ~NonConstantFieldReferenceExprError() override = default;
  std::string get_text(const ShowUnificationErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* arg_type;
};

struct NonexistentFieldReferenceError : public UnificationError {
  NonexistentFieldReferenceError(const Token* at_token, const Type* arg_type, const Type* field_type) :
    at_token(at_token), arg_type(arg_type), field_type(field_type) {
    //
  }
  ~NonexistentFieldReferenceError() override = default;
  std::string get_text(const ShowUnificationErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* arg_type;
  const Type* field_type;
};

using BoxedUnificationError = std::unique_ptr<UnificationError>;
using UnificationErrors = std::vector<BoxedUnificationError>;

class TokenSourceMap {
public:
  struct SourceData {
    SourceData() : SourceData(nullptr, nullptr, nullptr) {
      //
    }
    SourceData(const std::string* source,
               const CodeFileDescriptor* file_descriptor,
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

class ShowUnificationErrors {
public:
  ShowUnificationErrors(const TypeToString& type_to_string) :
  type_to_string(type_to_string) {
    //
  }

  void show(const UnificationError& err, int64_t index, const TokenSourceMap& source_data) const;
  void show(const UnificationErrors& errs, const TokenSourceMap& source_data) const;

  const char* stylize(const char* code) const;
  std::string stylize(const std::string& str, const char* style) const;

private:
  static constexpr int context_amount = 50;

public:
  const TypeToString& type_to_string;
};

}