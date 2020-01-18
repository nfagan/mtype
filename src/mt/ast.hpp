#pragma once

#include <memory>
#include <vector>
#include <string_view>

namespace mt {

struct AstNode {
  AstNode() = default;
  virtual ~AstNode() = default;
};

struct Expr : public AstNode {
  Expr() = default;
  ~Expr() override = default;
};

struct Stmt : public AstNode {
  Stmt() = default;
  ~Stmt() override = default;
};

struct Def : public AstNode {
  Def() = default;
  ~Def() override = default;
};

using BoxedAstNode = std::unique_ptr<AstNode>;
using BoxedExpr = std::unique_ptr<Expr>;
using BoxedStmt = std::unique_ptr<Stmt>;
using BoxedDef = std::unique_ptr<Def>;

struct Block : public AstNode {
  Block() = default;
  ~Block() override = default;

  void append(BoxedAstNode other) {
    nodes.emplace_back(std::move(other));
  }

  std::vector<BoxedAstNode> nodes;
};

struct FunctionHeader {
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

  FunctionHeader header;
  std::unique_ptr<Block> body;
};

}