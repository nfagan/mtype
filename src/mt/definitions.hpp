#pragma once

#include "lang_components.hpp"
#include "token.hpp"
#include "handles.hpp"
#include "utility.hpp"
#include "ast/ast.hpp"
#include <unordered_map>
#include <cstdint>

namespace mt {

struct Import;

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

class MethodAttributes {
public:
  struct AttributeFlags {
    using Flag = uint8_t;

    static constexpr Flag is_abstract = 1u;
    static constexpr Flag is_sealed = 1u << 1u;
    static constexpr Flag is_static = 1u << 2u;
  };
public:
  MethodAttributes() : access_specifier(AccessSpecifier::public_access), boolean_attributes(0) {
    //
  }
  MethodAttributes(const ClassDefHandle& class_handle) : class_handle(class_handle) {
    //
  }

  void mark_abstract() {
    boolean_attributes |= AttributeFlags::is_abstract;
  }
  void mark_sealed() {
    boolean_attributes |= AttributeFlags::is_sealed;
  }
  void mark_static() {
    boolean_attributes |= AttributeFlags::is_static;
  }

  bool is_abstract() const {
    return boolean_attributes & AttributeFlags::is_abstract;
  }
  bool is_sealed() const {
    return boolean_attributes & AttributeFlags::is_sealed;
  }
  bool is_static() const {
    return boolean_attributes & AttributeFlags::is_static;
  }
  bool has_defining_class() const {
    return class_handle.is_valid();
  }

  AccessSpecifier get_access_specifier() const {
    return access_specifier;
  }

public:
  ClassDefHandle class_handle;

private:
  AccessSpecifier access_specifier;
  AttributeFlags::Flag boolean_attributes;
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
  FunctionDef(FunctionHeader&& header, std::unique_ptr<Block> body) :
    header(std::move(header)), body(std::move(body)) {
    //
  }
  FunctionDef(FunctionDef&& other) noexcept = default;
  FunctionDef& operator=(FunctionDef&& other) noexcept = default;

  FunctionHeader header;
  std::unique_ptr<Block> body;
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

struct ClassDef {
  struct Property {
    Property() = default;
    explicit Property(const MatlabIdentifier& name) : name(name) {
      //
    }
    ~Property() = default;

    MatlabIdentifier name;
  };

  struct MethodDef {
    MethodDef() = default;
    explicit MethodDef(const FunctionDefHandle& def_handle) : def_handle(def_handle) {
      //
    }
    FunctionDefHandle def_handle;
  };

  struct MethodDeclaration {
    MethodDeclaration() = default;
    explicit MethodDeclaration(FunctionHeader&& header) : header(std::move(header)) {
      //
    }

    FunctionHeader header;
  };

  using MethodDefs = std::vector<MethodDef>;
  using MethodDeclarations = std::vector<MethodDeclaration>;
  using Properties = std::vector<Property>;

  ClassDef() = default;
  ClassDef(const Token& source_token,
           const MatlabIdentifier& name,
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


}