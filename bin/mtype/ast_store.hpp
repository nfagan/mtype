#pragma once

#include "mt/mt.hpp"
#include <unordered_map>

namespace mt {

class AstStore {
public:
  struct Entry {
    Entry();
    Entry(BoxedRootBlock root_block, bool parsed, bool generated_constraints);

    BoxedRootBlock root_block;
    bool parsed_successfully;
    bool generated_type_constraints;
  };

  AstStore::Entry* insert(const FilePath& file_path, Entry&& entry);
  void emplace_parse_failure(const FilePath& file_path);

  Entry* lookup(const FilePath& file_path);
  const Entry* lookup(const FilePath& file_path) const;

  bool visited_file(const FilePath& for_file) const;
  bool successfully_parsed_file(const FilePath& for_file) const;
  bool generated_type_constraints(const FilePath& for_file);

  int64_t num_visited_files() const;

private:
  std::unordered_map<FilePath, Entry, FilePath::Hash> asts;
};

}