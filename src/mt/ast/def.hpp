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
  FunctionDef(FunctionDef&& other) noexcept = default;
  FunctionDef& operator=(FunctionDef&& other) noexcept = default;

  ~FunctionDef() override = default;
  std::string accept(const StringVisitor& vis) const override;

  FunctionHeader header;
  BoxedBlock body;
};

struct FunctionDefNode : public AstNode {
  FunctionDefNode(int64_t name, FunctionDefHandle def_handle, BoxedMatlabScope scope) :
  name(name), def_handle(def_handle), scope(std::move(scope)) {
    //
  }
  ~FunctionDefNode() override = default;

  std::string accept(const StringVisitor& vis) const override;
  FunctionDefNode* accept(IdentifierClassifier& classifier) override;

  int64_t name;
  FunctionDefHandle def_handle;
  BoxedMatlabScope scope;
};

struct FunctionReference {
  FunctionReference(int64_t name, FunctionDefHandle def_handle, BoxedMatlabScope scope) :
  name(name), def_handle(def_handle), scope(std::move(scope)) {
    //
  }

  int64_t name;
  FunctionDefHandle def_handle;
  BoxedMatlabScope scope;
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
    struct Less {
      bool operator()(const Property& a, const Property& b) const {
        return a.name < b.name;
      }
    };

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

  struct MethodDef {
    MethodDef() = default;
    MethodDef(std::unique_ptr<FunctionDefNode> def_node) : def_node(std::move(def_node)) {
      //
    }
    MethodDef(MethodDef&& other) noexcept = default;
    MethodDef& operator=(MethodDef&& other) noexcept = default;

    std::unique_ptr<FunctionDefNode> def_node;
  };

  struct MethodDeclaration {
    MethodDeclaration() = default;
    MethodDeclaration(FunctionHeader&& header) : header(std::move(header)) {
      //
    }
    MethodDeclaration(MethodDeclaration&& other) noexcept = default;
    MethodDeclaration& operator=(MethodDeclaration&& other) noexcept = default;

    FunctionHeader header;
  };

  using MethodDefs = std::vector<MethodDef>;
  using MethodDeclarations = std::vector<MethodDeclaration>;
  using Properties = std::set<Property, Property::Less>;

  ClassDef() = default;
  ClassDef(const Token& source_token,
           int64_t name,
           std::vector<int64_t>&& superclasses,
           Properties&& properties,
           MethodDefs&& method_defs,
           MethodDeclarations&& method_declarations) :
    source_token(source_token),
    name(name),
    superclass_names(std::move(superclasses)),
    properties(std::move(properties)),
    method_defs(std::move(method_defs)),
    method_declarations(std::move(method_declarations)) {
    //
  }
  ~ClassDef() override = default;
  ClassDef(ClassDef&& other) noexcept = default;
  ClassDef& operator=(ClassDef&& other) noexcept = default;

  std::string accept(const StringVisitor& vis) const override;

  Token source_token;
  int64_t name;
  std::vector<int64_t> superclass_names;
  Properties properties;
  MethodDefs method_defs;
  MethodDeclarations method_declarations;
};

class ClassDefHandle {
  friend class ClassStore;
public:
  ClassDefHandle() : index(-1) {
    //
  }
  ~ClassDefHandle() = default;

  bool is_valid() const {
    return index >= 0;
  }

private:
  ClassDefHandle(int64_t index) : index(index) {
    //
  }

  int64_t index;
};

struct ClassDefReference : public AstNode {
  ClassDefReference(const ClassDefHandle& handle) : handle(handle) {
    //
  }
  ~ClassDefReference() override = default;
  std::string accept(const StringVisitor& vis) const override;
  ClassDefReference* accept(IdentifierClassifier& classifier) override;

  ClassDefHandle handle;
};

}