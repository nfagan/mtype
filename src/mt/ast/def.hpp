#pragma once

#include "ast.hpp"
#include "../token.hpp"

namespace mt {

struct Block : public AstNode {
  Block() = default;
  ~Block() override = default;

  void append(BoxedAstNode other) {
    nodes.emplace_back(std::move(other));
  }
  std::string accept(const StringVisitor& vis) const override;

  std::vector<BoxedAstNode> nodes;
};

struct FunctionHeader {
  Token name_token;
  std::string_view name;
  std::vector<std::string_view> outputs;
  std::vector<std::string_view> inputs;
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