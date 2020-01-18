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

  std::vector<BoxedAstNode> nodes;
};

struct FunctionDef : public Def {
  FunctionDef(std::string_view name,
              std::vector<std::string_view>&& outputs,
              std::vector<std::string_view>&& inputs,
              std::unique_ptr<Block> body) :
              name(name), outputs(std::move(outputs)), inputs(std::move(inputs)), body(std::move(body)) {
    //
  }

  ~FunctionDef() override = default;

  std::string_view name;
  std::vector<std::string_view> outputs;
  std::vector<std::string_view> inputs;
  std::unique_ptr<Block> body;
};



}