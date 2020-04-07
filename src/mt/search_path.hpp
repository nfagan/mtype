#pragma once

#include "fs/path.hpp"
#include "fs/directory.hpp"
#include "utility.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mt {

struct SearchCandidate {
  SearchCandidate() : precedence(0) {
    //
  }

  SearchCandidate(int64_t precedence, FilePath defining_file) :
    precedence(precedence), defining_file(std::move(defining_file)) {
    //
  }

  int64_t precedence;
  FilePath defining_file;
  std::vector<SearchCandidate> alternates;
};

class SearchPath {
public:
  SearchPath() : is_within_private_directory(false), private_directory_parent(nullptr) {
    //
  }

  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(SearchPath)

  MT_NODISCARD DirectoryIterator::Status build(const std::vector<FilePath>& directories);

  Optional<const SearchCandidate*> search_for(const std::string& name) const;
  Optional<const SearchCandidate*> search_for(const std::string& name, const FilePath& from_directory) const;

  int64_t size() const;

  static Optional<SearchPath> build_from_path_file(const FilePath& file);

private:
  using CandidateMap = std::unordered_map<std::string, SearchCandidate>;
  using Status = DirectoryIterator::Status;

  MT_NODISCARD Status build_one(const FilePath& path, int64_t precedence, const std::string& parent_package);

  void maybe_add_file(const FilePath& path, int64_t precedence,
                      const std::string& parent_package, const DirectoryEntry& entry);

  Status traverse_package(const FilePath& path, int64_t precedence,
                          const std::string& parent_package, const DirectoryEntry& entry);
  Status traverse_private_directory(const FilePath& path, int64_t precedence,
                                    const std::string& parent_package, const DirectoryEntry& entry);

  CandidateMap& require_private_candidate_map(const FilePath& for_private_directory_parent);

  static void add_to_candidate_map(SearchPath::CandidateMap& files,
                                   const std::string& candidate_name,
                                   const SearchCandidate& candidate);

  static Optional<const SearchCandidate*> search_for(const SearchPath::CandidateMap& map, const std::string& key);


private:
  CandidateMap candidate_files;
  std::unordered_map<FilePath, CandidateMap, FilePath::Hash> private_candidates;

  bool is_within_private_directory;
  const FilePath* private_directory_parent;
};

}