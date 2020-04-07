#pragma once

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include "../handles.hpp"
#include "../Optional.hpp"

namespace mt {

class StringVisitor;
class TypePreservingVisitor;
class IdentifierClassifier;

struct Block;
struct RootBlock;
struct MatlabScope;
struct FunctionDefNode;

struct AstNode {
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual std::string accept(const StringVisitor& vis) const = 0;
  virtual AstNode* accept(IdentifierClassifier& classifier) = 0;

  virtual void accept(TypePreservingVisitor&) = 0;
  virtual void accept_const(TypePreservingVisitor&) const = 0;

  virtual bool represents_class_def() const {
    return false;
  }
  virtual bool represents_function_def() const {
    return false;
  }
  virtual bool represents_stmt_or_stmts() const {
    return false;
  }
  virtual bool represents_type_annot_macro() const {
    return false;
  }
  virtual Optional<AstNode*> enclosed_code_ast_node() const {
    return NullOpt{};
  }
};

template <typename T>
struct is_ast_node : public std::false_type {};
template<>
struct is_ast_node<AstNode> : public std::true_type {};

struct Expr : public AstNode {
  Expr() = default;
  ~Expr() override = default;

  void accept(TypePreservingVisitor& vis) override = 0;
  void accept_const(TypePreservingVisitor& vis) const override = 0;

  std::string accept(const StringVisitor& vis) const override = 0;
  Expr* accept(IdentifierClassifier&) override {
    return this;
  }

  virtual bool is_grouping_expr() const {
    return false;
  }

  virtual bool is_literal_field_reference_expr() const {
    return false;
  }

  virtual bool is_identifier_reference_expr() const {
    return false;
  }

  virtual bool is_static_identifier_reference_expr() const {
    return false;
  }

  virtual bool is_valid_assignment_target() const {
    return false;
  }

  virtual bool is_anonymous_function_expr() const {
    return false;
  }

  virtual bool append_to_compound_identifier(std::vector<int64_t>&) const {
    return false;
  }
};

struct Stmt : public AstNode {
  Stmt() = default;
  ~Stmt() override = default;

  void accept(TypePreservingVisitor& vis) override = 0;
  void accept_const(TypePreservingVisitor& vis) const override = 0;

  virtual bool is_assignment_stmt() const {
    return false;
  }

  bool represents_stmt_or_stmts() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override = 0;
  Stmt* accept(IdentifierClassifier&) override {
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

  virtual void accept(TypePreservingVisitor&) override {}
  virtual void accept_const(TypePreservingVisitor&) const override {}
};

struct TypeNode : public TypeAnnot {
  TypeNode() = default;
  ~TypeNode() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
};

using BoxedAstNode = std::unique_ptr<AstNode>;
using BoxedExpr = std::unique_ptr<Expr>;
using BoxedStmt = std::unique_ptr<Stmt>;
using BoxedTypeAnnot = std::unique_ptr<TypeAnnot>;
using BoxedType = std::unique_ptr<TypeNode>;
using BoxedBlock = std::unique_ptr<Block>;
using BoxedRootBlock = std::unique_ptr<RootBlock>;

struct Block : public AstNode {
  Block() = default;
  ~Block() override = default;

  bool represents_stmt_or_stmts() const override {
    return true;
  }

  void append(BoxedAstNode other) {
    nodes.emplace_back(std::move(other));
  }

  void append_many(std::vector<BoxedAstNode>& new_nodes) {
    for (auto& node : new_nodes) {
      append(std::move(node));
    }
  }

  std::string accept(const StringVisitor& vis) const override;
  Block* accept(IdentifierClassifier& classifier) override;
  virtual void accept(TypePreservingVisitor& vis) override;
  virtual void accept_const(TypePreservingVisitor&) const override;

  std::vector<BoxedAstNode> nodes;
};

struct RootBlock : public AstNode {
  RootBlock(BoxedBlock block, MatlabScope* scope) :
  block(std::move(block)), scope(scope) {
    //
  }
  ~RootBlock() override = default;

  bool represents_stmt_or_stmts() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  RootBlock* accept(IdentifierClassifier& classifier) override;

  virtual void accept(TypePreservingVisitor& vis) override;
  virtual void accept_const(TypePreservingVisitor&) const override;

  Optional<FunctionDefNode*> top_level_function_def() const;

  BoxedBlock block;
  MatlabScope* scope;
};

}