#pragma once

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>

namespace mt {

class StringVisitor;
class IdentifierClassifier;
struct Block;
struct FunctionDef;
struct FunctionReference;
struct VariableDef;
struct MatlabScope;
struct Import;

struct AstNode {
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual std::string accept(const StringVisitor& vis) const = 0;
  virtual AstNode* accept(IdentifierClassifier& classifier) = 0;
};

template <typename T>
struct is_ast_node : public std::false_type {};
template<>
struct is_ast_node<AstNode> : public std::true_type {};

struct Expr : public AstNode {
  Expr() = default;
  ~Expr() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  Expr* accept(IdentifierClassifier&) override {
    return this;
  }

  virtual bool is_valid_assignment_target() const {
    return false;
  }

  virtual bool append_to_compound_identifier(std::vector<int64_t>&) const {
    return false;
  }
};

struct Stmt : public AstNode {
  Stmt() = default;
  ~Stmt() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  Stmt* accept(IdentifierClassifier&) override {
    return this;
  }
};

struct Def : public AstNode {
  Def() = default;
  ~Def() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  Def* accept(IdentifierClassifier&) override {
    return this;
  }
};

struct TypeAnnot : public AstNode {
  TypeAnnot() = default;
  ~TypeAnnot() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
  TypeAnnot* accept(IdentifierClassifier&) override {
    return this;
  }
};

struct Type : public TypeAnnot {
  Type() = default;
  ~Type() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
};

using BoxedAstNode = std::unique_ptr<AstNode>;
using BoxedExpr = std::unique_ptr<Expr>;
using BoxedStmt = std::unique_ptr<Stmt>;
using BoxedDef = std::unique_ptr<Def>;
using BoxedTypeAnnot = std::unique_ptr<TypeAnnot>;
using BoxedType = std::unique_ptr<Type>;
using BoxedBlock = std::unique_ptr<Block>;
using BoxedMatlabScope = std::shared_ptr<MatlabScope>;

struct Block : public AstNode {
  Block() = default;
  ~Block() override = default;

  void append(BoxedAstNode other) {
    nodes.emplace_back(std::move(other));
  }

  void append_many(std::vector<BoxedAstNode>& nodes) {
    for (auto& node : nodes) {
      append(std::move(node));
    }
  }

  std::string accept(const StringVisitor& vis) const override;
  Block* accept(IdentifierClassifier& classifier) override;

  std::vector<BoxedAstNode> nodes;
};

struct RootBlock : public AstNode {
  RootBlock(BoxedBlock block, std::shared_ptr<MatlabScope> scope) :
  block(std::move(block)), scope(std::move(scope)) {
    //
  }
  ~RootBlock() override = default;
  std::string accept(const StringVisitor& vis) const override;
  RootBlock* accept(IdentifierClassifier& classifier) override;

  BoxedBlock block;
  std::shared_ptr<MatlabScope> scope;
};

struct FunctionReference : public AstNode {
  FunctionReference(int64_t name, FunctionDef* def, std::shared_ptr<MatlabScope> scope) :
    name(name), def(def), scope(std::move(scope)) {
    //
  }
  ~FunctionReference() override = default;
  std::string accept(const StringVisitor& vis) const override;
  FunctionReference* accept(IdentifierClassifier& classifier) override;

  int64_t name;
  FunctionDef* def;
  std::shared_ptr<MatlabScope> scope;
};

/*
 * MatlabScope
 */

struct MatlabScope {
  explicit MatlabScope(std::shared_ptr<MatlabScope> parent) : parent(std::move(parent)) {
    //
  }
  ~MatlabScope() = default;

  bool register_local_function(int64_t name, FunctionReference* ref);
  void register_local_variable(int64_t name, std::unique_ptr<VariableDef> def);
  void register_import(Import&& import);

  std::shared_ptr<MatlabScope> parent;
  std::unordered_map<int64_t, FunctionReference*> local_functions;
  std::unordered_map<int64_t, std::unique_ptr<VariableDef>> local_variables;
  std::vector<Import> fully_qualified_imports;
  std::vector<Import> wildcard_imports;
};

}