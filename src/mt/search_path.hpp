#pragma once

#include "fs/path.hpp"
#include "fs/directory.hpp"
#include "utility.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mt {

class CodeFileDescriptor;

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
  friend struct SearchPathBuilder;
public:
  using CandidateMap = std::unordered_map<std::string, SearchCandidate>;

  SearchPath() = default;
  SearchPath(SearchPath&& other) MSVC_MISSING_NOEXCEPT = default;
  SearchPath& operator=(SearchPath&& other) MSVC_MISSING_NOEXCEPT = default;

  Optional<const SearchCandidate*> search_for(const std::string& name) const;
  Optional<const SearchCandidate*> search_for(const std::string& name,
                                              const FilePath& from_directory) const;
  Optional<const SearchCandidate*> search_for(const std::string& name,
                                              const CodeFileDescriptor& file_descriptor) const;

  int64_t size() const;

private:
  CandidateMap& require_private_candidate_map(const FilePath& for_private_directory_parent);
  static Optional<const SearchCandidate*> search_for(const SearchPath::CandidateMap& map, const std::string& key);

private:
  CandidateMap candidate_files;
  std::unordered_map<FilePath, CandidateMap, FilePath::Hash> private_candidates;
};

Optional<SearchPath> build_search_path_from_path_file(const FilePath& file);
Optional<SearchPath> build_search_path_from_paths(const std::vector<FilePath>& directories);

}