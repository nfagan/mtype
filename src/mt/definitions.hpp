#pragma once

#include "lang_components.hpp"
#include "token.hpp"
#include "handles.hpp"
#include "utility.hpp"
#include "ast/ast.hpp"
#include "identifier.hpp"
#include <unordered_map>
#include <cstdint>

namespace mt {

struct Import;

/*
* MatlabScope
*/

struct MatlabScope {
  explicit MatlabScope(const MatlabScopeHandle& parent) : parent(parent) {
    //
  }

  ~MatlabScope() = default;

  bool register_local_function(const MatlabIdentifier& name, const FunctionReferenceHandle& handle);
  bool register_class(const MatlabIdentifier& name, const ClassDefHandle& handle);
  void register_local_variable(const MatlabIdentifier& name, const VariableDefHandle& handle);
  void register_import(Import&& import);

  MatlabScopeHandle parent;
  std::unordered_map<MatlabIdentifier, FunctionReferenceHandle, MatlabIdentifier::Hash> local_functions;
  std::unordered_map<MatlabIdentifier, VariableDefHandle, MatlabIdentifier::Hash> local_variables;
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

struct AccessSpecifier {
  AccessSpecifier() : type(AccessType::private_access), num_classes(0) {
    //
  }

  AccessSpecifier(AccessType type, std::unique_ptr<MatlabIdentifier[]> classes, int64_t num_classes) :
  type(type), classes(std::move(classes)), num_classes(num_classes) {
    //
  }
  AccessSpecifier(AccessSpecifier&& other) noexcept = default;
  AccessSpecifier& operator=(AccessSpecifier&& other) noexcept = default;

  ~AccessSpecifier() = default;

  AccessType type;
  std::unique_ptr<MatlabIdentifier[]> classes;
  int64_t num_classes;
};

class FunctionAttributes {
public:
  struct AttributeFlags {
    using Flag = uint8_t;

    static constexpr Flag is_abstract_method = 1u;
    static constexpr Flag is_hidden_method = 1u << 1u;
    static constexpr Flag is_sealed_method = 1u << 2u;
    static constexpr Flag is_static_method = 1u << 3u;
  };
public:
  FunctionAttributes() : FunctionAttributes(ClassDefHandle()) {
    //
  }
  FunctionAttributes(const ClassDefHandle& class_handle) :
  class_handle(class_handle), access_type(AccessType::public_access), boolean_attributes(0) {
    //
  }

  void mark_boolean_attribute_from_name(std::string_view name);

  void mark_abstract() {
    boolean_attributes |= AttributeFlags::is_abstract_method;
  }
  void mark_hidden() {
    boolean_attributes |= AttributeFlags::is_hidden_method;
  }
  void mark_sealed() {
    boolean_attributes |= AttributeFlags::is_sealed_method;
  }
  void mark_static() {
    boolean_attributes |= AttributeFlags::is_static_method;
  }

  bool is_abstract() const {
    return boolean_attributes & AttributeFlags::is_abstract_method;
  }
  bool is_hidden() const {
    return boolean_attributes & AttributeFlags::is_hidden_method;
  }
  bool is_sealed() const {
    return boolean_attributes & AttributeFlags::is_sealed_method;
  }
  bool is_static() const {
    return boolean_attributes & AttributeFlags::is_static_method;
  }
  bool is_class_method() const {
    return class_handle.is_valid();
  }

public:
  ClassDefHandle class_handle;
  AccessType access_type;

private:
  AttributeFlags::Flag boolean_attributes;
};

struct FunctionHeader {
  FunctionHeader() = default;
  FunctionHeader(const Token& name_token,
                 const MatlabIdentifier& name,
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
  MatlabIdentifier name;
  std::vector<int64_t> outputs;
  std::vector<FunctionInputParameter> inputs;
};

struct FunctionDef {
  FunctionDef(FunctionHeader&& header, const FunctionAttributes& attrs) :
  header(std::move(header)), attributes(attrs), body(nullptr) {
    //
  }

  FunctionDef(FunctionHeader&& header, const FunctionAttributes& attrs, std::unique_ptr<Block> body) :
    header(std::move(header)), attributes(attrs), body(std::move(body)) {
    //
  }
  FunctionDef(FunctionDef&& other) noexcept = default;
  FunctionDef& operator=(FunctionDef&& other) noexcept = default;

  bool is_declaration() const {
    return body == nullptr;
  }

  FunctionHeader header;
  FunctionAttributes attributes;
  std::unique_ptr<Block> body;
};

struct FunctionReference {
  FunctionReference(const MatlabIdentifier& name,
                    FunctionDefHandle def_handle,
                    const MatlabScopeHandle& scope_handle) :
    name(name), def_handle(def_handle), scope_handle(scope_handle) {
    //
  }

  MatlabIdentifier name;
  FunctionDefHandle def_handle;
  MatlabScopeHandle scope_handle;
};

struct VariableDef {
  explicit VariableDef(const MatlabIdentifier& name) : name(name) {
    //
  }

  MatlabIdentifier name;
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
  using Properties = std::vector<Property>;
  using Methods = std::vector<FunctionDefHandle>;

  ClassDef() = default;
  ClassDef(const Token& source_token,
           const MatlabIdentifier& name,
           std::vector<MatlabIdentifier>&& superclasses,
           Properties&& properties,
           Methods&& methods) :
    source_token(source_token),
    name(name),
    superclass_names(std::move(superclasses)),
    properties(std::move(properties)),
    methods(std::move(methods)) {
    //
  }
  ~ClassDef() = default;
  ClassDef(ClassDef&& other) MSVC_MISSING_NOEXCEPT = default;
  ClassDef& operator=(ClassDef&& other) MSVC_MISSING_NOEXCEPT = default;

  Token source_token;
  MatlabIdentifier name;
  std::vector<MatlabIdentifier> superclass_names;
  Properties properties;
  Methods methods;
};


}