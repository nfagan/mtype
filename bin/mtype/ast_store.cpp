#include "ast_store.hpp"

namespace mt {

AstStore::Entry::Entry() :
parsed_successfully(false), generated_type_constraints(false), resolved_type_identifiers(false) {
  //
}

AstStore::Entry::Entry(BoxedRootBlock root_block, const ClassDefHandle& maybe_class_def,
                       const FunctionReferenceHandle& maybe_function_ref) :
  root_block(std::move(root_block)),
  parsed_successfully(true),
  generated_type_constraints(false),
  resolved_type_identifiers(false),
  file_entry_class_def(maybe_class_def),
  file_entry_function_ref(maybe_function_ref) {
  //
}

AstStore::Entry* AstStore::insert(const FilePath& file_path, AstStore::Entry&& entry) {
  asts[file_path] = std::move(entry);
  return &asts.at(file_path);
}

void AstStore::emplace_parse_failure(const FilePath& file_path) {
  asts[file_path] = Entry();
}

bool AstStore::visited_file(const FilePath& for_file) const {
  return lookup(for_file) != nullptr;
}

bool AstStore::successfully_parsed_file(const FilePath& for_file) const {
  const auto maybe_entry = lookup(for_file);
  if (!maybe_entry) {
    return false;
  } else {
    return maybe_entry->parsed_successfully;
  }
}

bool AstStore::generated_type_constraints(const FilePath& for_file) {
  const auto maybe_entry = lookup(for_file);
  if (!maybe_entry) {
    return false;
  } else {
    return maybe_entry->generated_type_constraints;
  }
}

int64_t AstStore::num_visited_files() const {
  return asts.size();
}

AstStore::Entry* AstStore::lookup(const FilePath& file_path) {
  auto it = asts.find(file_path);
  return it == asts.end() ? nullptr : &it->second;
}

const AstStore::Entry* AstStore::lookup(const FilePath& file_path) const {
  auto it = asts.find(file_path);
  return it == asts.end() ? nullptr : &it->second;
}

}
