#pragma once

#include "ast.hpp"
#include "../token.hpp"

namespace mt {

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
  FunctionDef(FunctionHeader&& header, std::unique_ptr<Block> body) :
    header(std::move(header)), body(std::move(body)) {
    //
  }

  ~FunctionDef() override = default;
  std::string accept(const StringVisitor& vis) const override;

  FunctionHeader header;
  std::unique_ptr<Block> body;
};

}