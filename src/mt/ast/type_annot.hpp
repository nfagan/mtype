#pragma once

#include "ast.hpp"
#include "../token.hpp"

namespace mt {

struct TypeAnnotMacro : public TypeAnnot {
  TypeAnnotMacro(const Token& source_token, BoxedTypeAnnot annotation) :
  source_token(source_token), annotation(std::move(annotation)) {
    //
  }
  ~TypeAnnotMacro() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  BoxedTypeAnnot annotation;
};

struct InlineType : public TypeAnnot {
  explicit InlineType(BoxedType type) : type(std::move(type)) {
    //
  }
  ~InlineType() override = default;
  std::string accept(const StringVisitor& vis) const override;

  BoxedType type;
};

struct TypeLet : public TypeAnnot {
  TypeLet(const Token& source_token,
          int64_t identifier,
          BoxedType equal_to_type) :
          source_token(source_token),
          identifier(identifier),
          equal_to_type(std::move(equal_to_type)) {
    //
  }
  ~TypeLet() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  int64_t identifier;
  BoxedType equal_to_type;
};

struct TypeGiven : public TypeAnnot {
  TypeGiven(const Token& source_token,
            std::vector<int64_t>&& identifiers,
            BoxedTypeAnnot declaration) :
            source_token(source_token),
            identifiers(std::move(identifiers)),
            declaration(std::move(declaration)) {
    //
  }
  ~TypeGiven() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  std::vector<int64_t> identifiers;
  BoxedTypeAnnot declaration;
};

struct TypeBegin : public TypeAnnot {
  TypeBegin(const Token& source_token, bool is_exported, std::vector<BoxedTypeAnnot>&& contents) :
  source_token(source_token), is_exported(is_exported), contents(std::move(contents)) {
    //
  }
  ~TypeBegin() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  bool is_exported;
  std::vector<BoxedTypeAnnot> contents;
};

struct UnionType : public TypeNode {
  explicit UnionType(std::vector<BoxedType>&& members) : members(std::move(members)) {
    //
  }
  ~UnionType() override = default;
  std::string accept(const StringVisitor& vis) const override;

  std::vector<BoxedType> members;
};

struct ScalarType : public TypeNode {
  ScalarType(const Token& source_token,
             int64_t identifier,
             std::vector<BoxedType>&& arguments) :
             source_token(source_token),
             identifier(identifier),
             arguments(std::move(arguments)) {
    //
  }
  ~ScalarType() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  int64_t identifier;
  std::vector<BoxedType> arguments;
};

struct FunctionType : public TypeNode {
  FunctionType(const Token& source_token,
               std::vector<BoxedType>&& outputs,
               std::vector<BoxedType>&& inputs) :
               source_token(source_token),
               outputs(std::move(outputs)),
               inputs(std::move(inputs)) {
    //
  }
  ~FunctionType() override = default;
  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  std::vector<BoxedType> outputs;
  std::vector<BoxedType> inputs;
};

}