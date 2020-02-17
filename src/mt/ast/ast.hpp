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
struct RootBlock;
struct FunctionDef;
struct FunctionReference;
struct VariableDef;
struct MatlabScope;
struct Import;
class ClassDefHandle;
class MatlabIdentifier;
class ScopeStore;

struct AstNode {
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual std::string accept(const StringVisitor& vis) const = 0;
  virtual AstNode* accept(IdentifierClassifier& classifier) = 0;

  virtual bool represents_class_def() const {
    return false;
  }
  virtual bool represents_function_def() const {
    return false;
  }
  virtual bool represents_stmt_or_stmts() const {
    return false;
  }
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

  virtual bool append_to_compound_identifier(std::vector<int64_t>&) const {
    return false;
  }
};

struct Stmt : public AstNode {
  Stmt() = default;
  ~Stmt() override = default;

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
};

struct Type : public TypeAnnot {
  Type() = default;
  ~Type() override = default;

  std::string accept(const StringVisitor& vis) const override = 0;
};

using BoxedAstNode = std::unique_ptr<AstNode>;
using BoxedExpr = std::unique_ptr<Expr>;
using BoxedStmt = std::unique_ptr<Stmt>;
using BoxedTypeAnnot = std::unique_ptr<TypeAnnot>;
using BoxedType = std::unique_ptr<Type>;
using BoxedBlock = std::unique_ptr<Block>;
using BoxedRootBlock = std::unique_ptr<RootBlock>;

namespace detail {
  template <int Disambiguator>
  class Handle {
  public:
    Handle() : index(-1) {
      //
    }

    bool is_valid() const {
      return index >= 0;
    }

    int64_t get_index() const {
      return index;
    }

  protected:
    explicit Handle(int64_t index) : index(index) {
      //
    }

    int64_t index;
  };
}

class FunctionDefHandle : public detail::Handle<0> {
public:
  friend class Store;
  using Handle::Handle;
  using Handle::is_valid;
  using Handle::get_index;
};

class FunctionReferenceHandle : public detail::Handle<1> {
public:
  friend class Store;
  using Handle::Handle;
  using Handle::is_valid;
  using Handle::get_index;
};

class VariableDefHandle : public detail::Handle<2> {
public:
  friend class Store;
  using Handle::Handle;
  using Handle::is_valid;
  using Handle::get_index;
};

class MatlabScopeHandle : public detail::Handle<3> {
public:
  friend class Store;
  friend class ScopeStore;
  using Handle::Handle;
  using Handle::is_valid;
  using Handle::get_index;
};

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

  std::vector<BoxedAstNode> nodes;
};

struct RootBlock : public AstNode {
  RootBlock(BoxedBlock block, const MatlabScopeHandle& scope_handle) :
  block(std::move(block)), scope_handle(scope_handle) {
    //
  }
  ~RootBlock() override = default;

  bool represents_stmt_or_stmts() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  RootBlock* accept(IdentifierClassifier& classifier) override;

  BoxedBlock block;
  MatlabScopeHandle scope_handle;
};

/*
 * MatlabIdentifier
 */

class MatlabIdentifier {
  friend struct Hash;
public:
  struct Hash {
    std::size_t operator()(const MatlabIdentifier& k) const;
  };

public:
  MatlabIdentifier() : MatlabIdentifier(-1, 0) {
    //
  }

  explicit MatlabIdentifier(int64_t name) : MatlabIdentifier(name, 1) {
    //
  }

  MatlabIdentifier(int64_t name, int num_components) :
    name(name), num_components(num_components) {
    //
  }

  bool operator==(const MatlabIdentifier& other) const {
    return name == other.name;
  }

  bool is_valid() const {
    return size() > 0;
  }

  bool is_compound() const {
    return num_components > 1;
  }

  int size() const {
    return num_components;
  }

  int64_t full_name() const {
    return name;
  }

private:
  int64_t name;
  int num_components;
};

/*
 * MatlabScope
 */

struct MatlabScope {
  explicit MatlabScope(const MatlabScopeHandle& parent) : parent(parent) {
    //
  }

  ~MatlabScope() = default;

  bool register_local_function(int64_t name, const FunctionReferenceHandle& handle);
  bool register_class(const MatlabIdentifier& name, const ClassDefHandle& handle);
  void register_local_variable(int64_t name, const VariableDefHandle& handle);
  void register_import(Import&& import);

  MatlabScopeHandle parent;
  std::unordered_map<int64_t, FunctionReferenceHandle> local_functions;
  std::unordered_map<int64_t, VariableDefHandle> local_variables;
  std::unordered_map<MatlabIdentifier, ClassDefHandle, MatlabIdentifier::Hash> classes;
  std::vector<Import> fully_qualified_imports;
  std::vector<Import> wildcard_imports;
};

}