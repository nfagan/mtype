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

}