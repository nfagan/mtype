#pragma once

#include "components.hpp"
#include "../utility.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstddef>

namespace mt {

struct Type;
template <typename T>
class Optional;

struct TypeScope;

struct TypeImport {
  struct Hash {
    std::size_t operator()(const TypeImport& a) const;
  };

  TypeImport(TypeScope* root, bool is_exported) : root(root), is_exported(is_exported) {
    //
  }

  friend inline bool operator==(const TypeImport& a, const TypeImport& b) {
    return a.root == b.root;
  }
  friend inline bool operator!=(const TypeImport& a, const TypeImport& b) {
    return !(a == b);
  }

  TypeScope* root;
  bool is_exported;
};

/*
 * TypeScope
 */

struct TypeScope {
  using TypeImports = std::unordered_set<TypeImport, TypeImport::Hash>;
  using TypeMap = std::unordered_map<TypeIdentifier, TypeReference*, TypeIdentifier::Hash>;

  TypeScope(TypeScope* root, const TypeScope* parent) : root(root), parent(parent) {
    //
  }

  bool is_root() const;
  void add_child(TypeScope* child);
  void add_import(const TypeImport& import);

  MT_NODISCARD Optional<TypeReference*> lookup_type(const TypeIdentifier& ident) const;
  void emplace_type(const TypeIdentifier& ident, TypeReference* ref, bool is_export);

  bool can_register_local_identifier(const TypeIdentifier& ident, bool is_export);
  MT_NODISCARD Optional<TypeReference*> registered_local_identifier(const TypeIdentifier& ident, bool is_export) const;

private:
  bool has_locally_defined_type(const TypeIdentifier& ident) const;

  void emplace_locally_defined_type(const TypeIdentifier& ident, TypeReference* ref);
  void emplace_exported_type(const TypeIdentifier& ident, TypeReference* ref);

  MT_NODISCARD Optional<TypeReference*> lookup_local_type(const TypeIdentifier& ident, bool traverse_parent) const;
  MT_NODISCARD Optional<TypeReference*> lookup_exported_type(const TypeIdentifier& ident) const;

public:
  TypeScope* root;
  const TypeScope* parent;
  std::vector<TypeScope*> children;

  TypeMap local_types;
  TypeMap exports;
  TypeImports imports;
};

}