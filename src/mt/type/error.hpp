#pragma once

#include "type.hpp"
#include "../Optional.hpp"
#include "../token.hpp"
#include <string>
#include <vector>

namespace mt {

class TypeStore;
class TypeToString;
class ShowTypeErrors;
class TokenSourceMap;

struct TypeError {
  virtual std::string get_text(const ShowTypeErrors& shower) const = 0;
  virtual Token get_source_token() const = 0;
  virtual ~TypeError() = default;
};

struct SimplificationFailure : public TypeError {
  SimplificationFailure(const Token* lhs_token, const Token* rhs_token,
                        const Type* lhs_type, const Type* rhs_type) :
  lhs_token(lhs_token), rhs_token(rhs_token), lhs_type(lhs_type), rhs_type(rhs_type) {
    //
  }
  ~SimplificationFailure() override = default;

  std::string get_text(const ShowTypeErrors& shower) const override;
  Token get_source_token() const override;

  const Token* lhs_token;
  const Token* rhs_token;
  const Type* lhs_type;
  const Type* rhs_type;
};

struct OccursCheckFailure : public TypeError {
  OccursCheckFailure(const Token* lhs_token, const Token* rhs_token,
                     const Type* lhs_type, const Type* rhs_type) :
  lhs_token(lhs_token), rhs_token(rhs_token), lhs_type(lhs_type), rhs_type(rhs_type) {
    //
  }
  ~OccursCheckFailure() override = default;

  std::string get_text(const ShowTypeErrors& shower) const override;
  Token get_source_token() const override;

  const Token* lhs_token;
  const Token* rhs_token;
  const Type* lhs_type;
  const Type* rhs_type;
};

struct UnresolvedFunctionError : public TypeError {
  UnresolvedFunctionError(const Token* at_token, const Type* function_type) :
  at_token(at_token), function_type(function_type) {
    //
  }
  ~UnresolvedFunctionError() override = default;
  std::string get_text(const ShowTypeErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* function_type;
};

struct InvalidFunctionInvocationError : public TypeError {
  InvalidFunctionInvocationError(const Token* at_token, const Type* function_type) :
  at_token(at_token), function_type(function_type) {
    //
  }
  ~InvalidFunctionInvocationError() override = default;
  std::string get_text(const ShowTypeErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* function_type;
};

struct NonConstantFieldReferenceExprError : public TypeError {
  NonConstantFieldReferenceExprError(const Token* at_token, const Type* arg_type) :
  at_token(at_token), arg_type(arg_type) {
    //
  }
  ~NonConstantFieldReferenceExprError() override = default;
  std::string get_text(const ShowTypeErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* arg_type;
};

struct NonexistentFieldReferenceError : public TypeError {
  NonexistentFieldReferenceError(const Token* at_token, const Type* arg_type, const Type* field_type) :
    at_token(at_token), arg_type(arg_type), field_type(field_type) {
    //
  }
  ~NonexistentFieldReferenceError() override = default;
  std::string get_text(const ShowTypeErrors& shower) const override;
  Token get_source_token() const override;

  const Token* at_token;
  const Type* arg_type;
  const Type* field_type;
};

struct DuplicateTypeIdentifierError : public TypeError {
  DuplicateTypeIdentifierError(const Token* initial_def, const Token* new_def) :
    initial_def(initial_def), new_def(new_def) {
    //
  }
  ~DuplicateTypeIdentifierError() override = default;
  std::string get_text(const ShowTypeErrors& shower) const override;
  Token get_source_token() const override;

  const Token* initial_def;
  const Token* new_def;
};

using BoxedTypeError = std::unique_ptr<TypeError>;
using TypeErrors = std::vector<BoxedTypeError>;

class ShowTypeErrors {
public:
  ShowTypeErrors(const TypeToString& type_to_string) :
  type_to_string(type_to_string) {
    //
  }

  void show(const TypeError& err, int64_t index, const TokenSourceMap& source_data) const;
  void show(const TypeErrors& errs, const TokenSourceMap& source_data) const;

  const char* stylize(const char* code) const;
  std::string stylize(const std::string& str, const char* style) const;

private:
  static constexpr int context_amount = 50;

public:
  const TypeToString& type_to_string;
};

}