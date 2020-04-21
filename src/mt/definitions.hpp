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
class CodeFileDescriptor;

/*
* MatlabScope
*/

struct MatlabScope {
  using FunctionMap = std::unordered_map<MatlabIdentifier, FunctionReferenceHandle, MatlabIdentifier::Hash>;
  using VariableMap = std::unordered_map<MatlabIdentifier, VariableDefHandle, MatlabIdentifier::Hash>;
  using ClassMap = std::unordered_map<MatlabIdentifier, ClassDefHandle, MatlabIdentifier::Hash>;

  explicit MatlabScope(const MatlabScope* parent, const CodeFileDescriptor* file_descriptor) :
  parent(parent), file_descriptor(file_descriptor) {
    //
  }

  ~MatlabScope() = default;

  bool register_local_function(const MatlabIdentifier& name, const FunctionReferenceHandle& handle);
  bool register_class(const MatlabIdentifier& name, const ClassDefHandle& handle);
  void register_local_variable(const MatlabIdentifier& name, const VariableDefHandle& handle);
  void register_import(Import&& import);
  void register_imported_function(const MatlabIdentifier& name, const FunctionReferenceHandle& handle);

  FunctionReferenceHandle lookup_local_function(const MatlabIdentifier& name) const;
  FunctionReferenceHandle lookup_imported_function(const MatlabIdentifier& name) const;

  bool has_imported_function(const MatlabIdentifier& name) const;

  const MatlabScope* parent;
  const CodeFileDescriptor* file_descriptor;

  FunctionMap local_functions;
  VariableMap local_variables;
  ClassMap classes;
  FunctionMap imported_functions;
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

struct FunctionParameter {
  struct AttributeFlags {
    using FlagType = uint8_t;
    static constexpr FlagType is_ignored = 1u;
    static constexpr FlagType is_vararg = 1u << 1u;
  };

  explicit FunctionParameter(const MatlabIdentifier& name) : name(name), flags(0) {
    //
  }
  FunctionParameter() : flags(0) {
    mark_is_ignored();
  }

  void mark_is_ignored() {
    flags |= AttributeFlags::is_ignored;
  }
  void mark_is_vararg() {
    flags |= AttributeFlags::is_vararg;
  }

  bool is_ignored() const {
    return flags & AttributeFlags::is_ignored;
  }

  bool is_vararg() const {
    return flags & AttributeFlags::is_vararg;
  }

  MatlabIdentifier name;
  AttributeFlags::FlagType flags;
};

using FunctionParameters = std::vector<FunctionParameter>;

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
    static constexpr Flag is_constructor = 1u << 4u;
    static constexpr Flag has_varargin = 1u << 5u;
    static constexpr Flag has_varargout = 1u << 6u;
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
  void mark_constructor() {
    boolean_attributes |= AttributeFlags::is_constructor;
  }
  void mark_has_varargin() {
    boolean_attributes |= AttributeFlags::has_varargin;
  }
  void mark_has_varargout() {
    boolean_attributes |= AttributeFlags::has_varargout;
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
  bool is_constructor() const {
    return boolean_attributes & AttributeFlags::is_constructor;
  }
  bool has_varargin() const {
    return boolean_attributes & AttributeFlags::has_varargin;
  }
  bool has_varargout() const {
    return boolean_attributes & AttributeFlags::has_varargout;
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
                 FunctionParameters&& outputs,
                 FunctionParameters&& inputs) :
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
  FunctionParameters outputs;
  FunctionParameters inputs;
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
  FunctionReference() : scope(nullptr) {
    //
  }

  FunctionReference(const MatlabIdentifier& name,
                    const FunctionDefHandle& def_handle,
                    const MatlabScope* scope) :
    name(name), def_handle(def_handle), scope(scope) {
    //
  }

  MatlabIdentifier name;
  FunctionDefHandle def_handle;
  const MatlabScope* scope;
};

struct VariableDef {
  explicit VariableDef(const MatlabIdentifier& name) : name(name) {
    //
  }

  MatlabIdentifier name;
};

struct ClassDef {
  struct Superclass {
    MatlabIdentifier name;
    ClassDefHandle def_handle;
  };

  struct Property {
    Property() = default;
    explicit Property(const MatlabIdentifier& name) : name(name) {
      //
    }

    MatlabIdentifier name;
  };

  using Properties = std::vector<Property>;
  using Methods = std::vector<FunctionDefHandle>;
  using Superclasses = std::vector<Superclass>;

  ClassDef() = default;
  ClassDef(const Token& source_token,
           const MatlabIdentifier& name,
           Superclasses&& superclasses,
           Properties&& properties,
           Methods&& methods) :
    source_token(source_token),
    name(name),
    superclasses(std::move(superclasses)),
    properties(std::move(properties)),
    methods(std::move(methods)) {
    //
  }
  ~ClassDef() = default;
  ClassDef(ClassDef&& other) MSVC_MISSING_NOEXCEPT = default;
  ClassDef& operator=(ClassDef&& other) MSVC_MISSING_NOEXCEPT = default;

  Token source_token;
  MatlabIdentifier name;
  Superclasses superclasses;
  Properties properties;
  Methods methods;
};


}