#pragma once

#include "mt/mt.hpp"
#include <unordered_map>

namespace mt {

class AstStore {
public:
  struct Entry {
    Entry();
    Entry(BoxedRootBlock root_block, const ClassDefHandle& maybe_class_def,
          const FunctionReferenceHandle& maybe_function_ref,
          CodeFileType file_type);

    BoxedRootBlock root_block;
    bool parsed_successfully;
    bool generated_type_constraints;
    bool resolved_type_identifiers;
    ClassDefHandle file_entry_class_def;
    FunctionReferenceHandle file_entry_function_ref;
    CodeFileType file_type;
  };

  AstStore::Entry* insert(const FilePath& file_path, Entry&& entry);
  bool remove(const FilePath& file_path);
  void emplace_parse_failure(const FilePath& file_path);

  Entry* lookup(const FilePath& file_path);
  const Entry* lookup(const FilePath& file_path) const;

  bool visited_file(const FilePath& for_file) const;
  bool successfully_parsed_file(const FilePath& for_file) const;
  bool generated_type_constraints(const FilePath& for_file);

  int64_t num_visited_files() const;

public:
  std::unordered_map<FilePath, Entry, FilePath::Hash> asts;
};

}