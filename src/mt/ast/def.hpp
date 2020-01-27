#pragma once

#include "ast.hpp"
#include "../token.hpp"

namespace mt {

struct MatlabScope;

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
              std::unique_ptr<Block> body,
              std::shared_ptr<MatlabScope> scope) :
    header(std::move(header)), body(std::move(body)), scope(std::move(scope)) {
    //
  }

  ~FunctionDef() override = default;
  std::string accept(const StringVisitor& vis) const override;
  Def* accept(IdentifierClassifier& classifier) override;

  FunctionHeader header;
  BoxedBlock body;
  std::shared_ptr<MatlabScope> scope;
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