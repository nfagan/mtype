#pragma once

#include "ast.hpp"
#include "../token.hpp"
#include "../lang_components.hpp"

namespace mt {

struct MatlabScope;

struct Import {
  Import() : type(ImportType::fully_qualified) {
    //
  }
  Import(const Token& source_token, ImportType type, std::vector<int64_t>&& identifier_components) :
  source_token(source_token), type(type), identifier_components(std::move(identifier_components)) {
    //
  }
  ~Import() = default;

  Token source_token;
  ImportType type;
  std::vector<int64_t> identifier_components;
};

struct FunctionHeader {
  FunctionHeader() = default;
  FunctionHeader(const Token& name_token,
                 int64_t name,
                 std::vector<int64_t>&& outputs,
                 std::vector<int64_t>&& inputs) :
                 name_token(name_token),
                 name(name),
                 outputs(std::move(outputs)),
                 inputs(std::move(inputs)) {
    //
  }
  FunctionHeader(FunctionHeader&& other) noexcept = default;
  FunctionHeader& operator=(FunctionHeader&& other) noexcept = default;
  ~FunctionHeader() = default;

  Token name_token;
  int64_t name;
  std::vector<int64_t> outputs;
  std::vector<int64_t> inputs;
};

struct FunctionDef : public Def {
  FunctionDef(FunctionHeader&& header,
              std::unique_ptr<Block> body) :
    header(std::move(header)), body(std::move(body)) {
    //
  }

  ~FunctionDef() override = default;
  std::string accept(const StringVisitor& vis) const override;

  FunctionHeader header;
  BoxedBlock body;
};

struct VariableDef : public Def {
  explicit VariableDef(int64_t name) : name(name) {
    //
  }
  ~VariableDef() override = default;
  std::string accept(const StringVisitor& vis) const override;

  int64_t name;
};

struct ClassDef : public Def {
  struct Property {
    Property() = default;
    Property(const Token& source_token, int64_t name, BoxedExpr initializer) :
    source_token(source_token), name(name), initializer(std::move(initializer)) {
      //
    }
    Property(Property&& other) noexcept = default;
    Property& operator=(Property&& other) noexcept = default;
    ~Property() = default;

    Token source_token;
    int64_t name;
    BoxedExpr initializer;
  };

  struct Properties {
    Properties() = default;
    Properties(const Token& source_token, std::vector<Property>&& properties) :
    source_token(source_token), properties(std::move(properties)) {
      //
    }
    Properties(Properties&& other) noexcept = default;
    Properties& operator=(Properties&& other) noexcept = default;
    ~Properties() = default;

    Token source_token;
    std::vector<Property> properties;
  };

  struct Methods {
    Methods() = default;
    Methods(const Token& source_token,
            std::vector<std::unique_ptr<FunctionReference>>&& local_methods,
            std::vector<FunctionHeader>&& external_methods) :
      source_token(source_token),
      definitions(std::move(local_methods)),
      declarations(std::move(external_methods)) {
      //
    }
    Methods(Methods&& other) noexcept = default;
    Methods& operator=(Methods&& other) noexcept = default;
    ~Methods() = default;

    Token source_token;
    std::vector<std::unique_ptr<FunctionReference>> definitions;
    std::vector<FunctionHeader> declarations;
  };

  ClassDef(const Token& source_token,
           int64_t name,
           std::vector<int64_t>&& superclasses,
           std::vector<Properties>&& property_blocks,
           std::vector<Methods>&& method_blocks) :
    source_token(source_token),
    name(name),
    superclass_names(std::move(superclasses)),
    property_blocks(std::move(property_blocks)),
    method_blocks(std::move(method_blocks)) {
    //
  }
  ~ClassDef() override = default;
  ClassDef(ClassDef&& other) noexcept = default;
  ClassDef& operator=(ClassDef&& other) noexcept = default;

  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  int64_t name;
  std::vector<int64_t> superclass_names;
  std::vector<Properties> property_blocks;
  std::vector<Methods> method_blocks;
};

}