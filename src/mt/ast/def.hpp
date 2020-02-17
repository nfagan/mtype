#pragma once

#include "ast.hpp"
#include "../token.hpp"
#include "../lang_components.hpp"
#include "../utility.hpp"

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

struct FunctionInputParameter {
  explicit FunctionInputParameter(int64_t name) : name(name), is_ignored(false) {
    //
  }

  FunctionInputParameter() : name(-1), is_ignored(true) {
    //
  }

  int64_t name;
  bool is_ignored;
};

struct FunctionHeader {
  FunctionHeader() = default;
  FunctionHeader(const Token& name_token,
                 int64_t name,
                 std::vector<int64_t>&& outputs,
                 std::vector<FunctionInputParameter>&& inputs) :
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
  std::vector<FunctionInputParameter> inputs;
};

struct FunctionDef {
  FunctionDef(FunctionHeader&& header, BoxedBlock body) :
    header(std::move(header)), body(std::move(body)) {
    //
  }
  FunctionDef(FunctionDef&& other) noexcept = default;
  FunctionDef& operator=(FunctionDef&& other) noexcept = default;

  FunctionHeader header;
  BoxedBlock body;
};

struct FunctionDefNode : public AstNode {
  FunctionDefNode(FunctionDefHandle def_handle, const MatlabScopeHandle& scope_handle) :
  def_handle(def_handle), scope_handle(scope_handle) {
    //
  }
  ~FunctionDefNode() override = default;

  bool represents_function_def() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  FunctionDefNode* accept(IdentifierClassifier& classifier) override;

  FunctionDefHandle def_handle;
  MatlabScopeHandle scope_handle;
};

struct FunctionReference {
  FunctionReference(int64_t name, FunctionDefHandle def_handle, const MatlabScopeHandle& scope_handle) :
  name(name), def_handle(def_handle), scope_handle(scope_handle) {
    //
  }

  int64_t name;
  FunctionDefHandle def_handle;
  MatlabScopeHandle scope_handle;
};

struct VariableDef {
  explicit VariableDef(int64_t name) : name(name) {
    //
  }

  int64_t name;
};

class ClassDefHandle {
  friend class Store;
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

struct ClassDef {
  struct Property {
    Property() = default;
    Property(const MatlabIdentifier& name) : name(name) {
      //
    }
    Property(Property&& other) MSVC_MISSING_NOEXCEPT = default;
    Property& operator=(Property&& other) MSVC_MISSING_NOEXCEPT = default;
    ~Property() = default;

    MatlabIdentifier name;
  };

  struct MethodDef {
    MethodDef() = default;
    MethodDef(const FunctionDefHandle& def_handle) : def_handle(def_handle) {
      //
    }
    MethodDef(MethodDef&& other) MSVC_MISSING_NOEXCEPT = default;
    MethodDef& operator=(MethodDef&& other) MSVC_MISSING_NOEXCEPT = default;

    FunctionDefHandle def_handle;
  };

  struct MethodDeclaration {
    MethodDeclaration() = default;
    MethodDeclaration(FunctionHeader&& header) : header(std::move(header)) {
      //
    }
    MethodDeclaration(MethodDeclaration&& other) MSVC_MISSING_NOEXCEPT = default;
    MethodDeclaration& operator=(MethodDeclaration&& other) MSVC_MISSING_NOEXCEPT = default;

    FunctionHeader header;
  };

  using MethodDefs = std::vector<MethodDef>;
  using MethodDeclarations = std::vector<MethodDeclaration>;
  using Properties = std::vector<Property>;

  ClassDef() = default;
  ClassDef(const Token& source_token,
           MatlabIdentifier name,
           std::vector<MatlabIdentifier>&& superclasses,
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
  ~ClassDef() = default;
  ClassDef(ClassDef&& other) MSVC_MISSING_NOEXCEPT = default;
  ClassDef& operator=(ClassDef&& other) MSVC_MISSING_NOEXCEPT = default;

  Token source_token;
  MatlabIdentifier name;
  std::vector<MatlabIdentifier> superclass_names;
  Properties properties;
  MethodDefs method_defs;
  MethodDeclarations method_declarations;
};

struct ClassDefNode : public AstNode {
public:
  struct Property {
    Property() = default;
    Property(const Token& source_token, int64_t name, BoxedExpr initializer) :
      source_token(source_token), name(name), initializer(std::move(initializer)) {
      //
    }
    Property(Property&& other) MSVC_MISSING_NOEXCEPT = default;
    Property& operator=(Property&& other) MSVC_MISSING_NOEXCEPT = default;
    ~Property() = default;

    Token source_token;
    int64_t name;
    BoxedExpr initializer;
  };

public:
  ClassDefNode(const Token& source_token,
               const ClassDefHandle& handle,
               std::vector<Property>&& properties,
               std::vector<std::unique_ptr<FunctionDefNode>>&& method_defs) :
               source_token(source_token),
               handle(handle),
               properties(std::move(properties)),
               method_defs(std::move(method_defs)) {
    //
  }
  ~ClassDefNode() override = default;

  bool represents_class_def() const override {
    return true;
  }

  std::string accept(const StringVisitor& vis) const override;
  ClassDefNode* accept(IdentifierClassifier& classifier) override;

  Token source_token;
  ClassDefHandle handle;
  std::vector<Property> properties;
  std::vector<std::unique_ptr<FunctionDefNode>> method_defs;
};


}