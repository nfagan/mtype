#include "type_scope.hpp"
#include "../Optional.hpp"

namespace mt {

/*
 * TypeImport
 */

std::size_t TypeImport::Hash::operator()(const TypeImport& a) const {
  return std::hash<const TypeScope*>{}(a.root);
}

/*
 * TypeScope
 */

void TypeScope::add_child(TypeScope* child) {
  children.push_back(child);
}

void TypeScope::add_import(const TypeImport& import) {
  imports.insert(import);

  if (import.is_exported && !is_root()) {
    root->add_import(import);
  }
}

bool TypeScope::is_root() const {
  return root == this;
}

bool TypeScope::has_locally_defined_type(const TypeIdentifier& ident) const {
  return local_types.count(ident) > 0;
}

Optional<TypeReference*> TypeScope::lookup_type(const TypeIdentifier& ident) const {
  auto maybe_local_def = lookup_local_type(ident, true);
  if (maybe_local_def) {
    return maybe_local_def;
  }

  std::unordered_set<const TypeScope*> visited{this};
  auto copy_imports = imports;
  auto it = copy_imports.begin();

  while (it != copy_imports.end()) {
    auto import = it++;
    if (visited.count(import->root) > 0) {
      continue;
    } else {
      visited.insert(import->root);
    }

    auto maybe_import = import->root->lookup_exported_type(ident);
    if (maybe_import) {
      return maybe_import;
    }

    for (const auto& transitive : import->root->imports) {
      if (transitive.is_exported) {
        copy_imports.insert(transitive);
      }
    }

    it = copy_imports.begin();
  }

  if (parent) {
    return parent->lookup_type(ident);
  }

  return NullOpt{};
}

Optional<TypeReference*> TypeScope::lookup_exported_type(const TypeIdentifier& ident) const {
  const auto it = root->exports.find(ident);
  if (it == root->exports.end()) {
    return NullOpt{};
  } else {
    return Optional<TypeReference*>(it->second);
  }
}

Optional<TypeReference*> TypeScope::lookup_local_type(const TypeIdentifier& ident, bool traverse_parent) const {
  const auto it = local_types.find(ident);
  if (it == local_types.end()) {
    return traverse_parent && parent ? parent->lookup_local_type(ident, traverse_parent) : NullOpt{};
  } else {
    return Optional<TypeReference*>(it->second);
  }
}

void TypeScope::emplace_locally_defined_type(const TypeIdentifier& ident, TypeReference* ref) {
  local_types[ident] = ref;
}

void TypeScope::emplace_exported_type(const TypeIdentifier& ident, TypeReference* ref) {
  root->exports[ident] = ref;
}

bool TypeScope::can_register_local_identifier(const TypeIdentifier& ident, bool is_export) {
  if (has_locally_defined_type(ident)) {
    return false;

  } else if (is_export) {
    auto maybe_dup_export = lookup_exported_type(ident);
    if (maybe_dup_export) {
      return false;
    }
  }

  return true;
}

Optional<TypeReference*> TypeScope::registered_local_identifier(const TypeIdentifier& ident, bool is_export) const {
  auto maybe_loc_type = lookup_local_type(ident, false);
  if (maybe_loc_type) {
    return maybe_loc_type;
  } else if (is_export) {
    return lookup_exported_type(ident);
  } else {
    return NullOpt{};
  }
}

void TypeScope::emplace_type(const TypeIdentifier& ident, TypeReference* ref, bool is_export) {
  emplace_locally_defined_type(ident, ref);

  if (is_export) {
    emplace_exported_type(ident, ref);
  }
}

}
