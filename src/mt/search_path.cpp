#include "search_path.hpp"
#include "fs/directory.hpp"
#include "unicode.hpp"
#include "character.hpp"
#include "string.hpp"
#include <cassert>
#include <fstream>

namespace mt {

namespace {
  void insert_parent_package(std::string parent_package, std::string& postfix) {
    parent_package += ".";
    parent_package += postfix;
    std::swap(parent_package, postfix);
  }

  void add_to_candidate_map(SearchPath::CandidateMap& files, const std::string& candidate_name,
                            const SearchCandidate& candidate) {
    const auto candidate_it = files.find(candidate_name);
    if (candidate_it == files.end()) {
      //  New name.
      files[candidate_name] = candidate;
    } else {
      //  Existing name.
      candidate_it->second.alternates.push_back(candidate);
    }
  }
}

/*
 * SearchPathBuilder
 */

struct SearchPathBuilder {
  explicit SearchPathBuilder(SearchPath* search_path) : search_path(search_path) {
    //
  }

  void maybe_add_file(const FilePath& path, int64_t precedence, const std::string& parent_package,
                      const DirectoryEntry& entry) const;

  SearchPath* search_path;
  bool is_within_private_directory = false;
  const FilePath* private_directory_parent = nullptr;
  bool is_within_class_directory = false;
  const std::string* class_directory_name = nullptr;
};

void SearchPathBuilder::maybe_add_file(const FilePath& path, int64_t precedence,
                                       const std::string& parent_package,
                                       const DirectoryEntry& entry) const {
  const std::size_t len_minus_ext = entry.name_size - 2;
  auto* maybe_dot_m = &entry.name[len_minus_ext];

  if (maybe_dot_m[0] == '.' && maybe_dot_m[1] == 'm') {
    std::string candidate_name{entry.name, len_minus_ext};
    FilePath candidate_file = fs::join(path, FilePath(entry.name));

    if (is_within_class_directory && candidate_name != *class_directory_name) {
      //  Only add the main class file in a class folder.
      return;
    }

    if (!parent_package.empty()) {
      insert_parent_package(parent_package, candidate_name);
    }

    SearchCandidate candidate(precedence, candidate_file);

    if (is_within_private_directory) {
      auto& private_map = search_path->require_private_candidate_map(*private_directory_parent);
      add_to_candidate_map(private_map, candidate_name, candidate);

    } else {
      add_to_candidate_map(search_path->candidate_files, candidate_name, candidate);
    }
  }
}


namespace {
  DirectoryIterator::Status build_one(SearchPathBuilder& builder, const FilePath& path,
                                      int64_t precedence, const std::string& parent_package);

  DirectoryIterator::Status traverse_private_directory(SearchPathBuilder& builder,
                                                       const FilePath& path, int64_t precedence,
                                                       const std::string& parent_package,
                                                       const DirectoryEntry& entry) {
    auto joined_path = fs::join(path, FilePath(entry.name));
    builder.private_directory_parent = &path;
    builder.is_within_private_directory = true;
    auto status = build_one(builder, joined_path, precedence, parent_package);
    builder.is_within_private_directory = false;
    builder.private_directory_parent = nullptr;
    return status;
  }

  DirectoryIterator::Status traverse_package(SearchPathBuilder& builder,
                                             const FilePath& path, int64_t precedence,
                                             const std::string& parent_package,
                                             const DirectoryEntry& entry) {
    //  Package path.
    auto joined_path = fs::join(path, FilePath(entry.name));
    std::string new_package{&entry.name[1]}; //  skip +

    if (!parent_package.empty()) {
      insert_parent_package(parent_package, new_package);
    }

    return build_one(builder, joined_path, precedence, new_package);
  }

  DirectoryIterator::Status traverse_class_directory(SearchPathBuilder& builder,
                                                     const FilePath& path,
                                                     int64_t precedence,
                                                     const std::string& parent_package,
                                                     const DirectoryEntry& entry) {
    auto joined_path = fs::join(path, FilePath(entry.name));
    std::string expect_class_name{&entry.name[1]};  //  skip @

    builder.class_directory_name = &expect_class_name;
    builder.is_within_class_directory = true;
    auto status = build_one(builder, joined_path, precedence, parent_package);
    builder.is_within_class_directory = false;
    builder.class_directory_name = nullptr;
    return status;
  }

  DirectoryIterator::Status build_one(SearchPathBuilder& builder, const FilePath& path,
                                      int64_t precedence, const std::string& parent_package) {
    if (!fs::directory_exists(path)) {
      //  Skip non-existent directories.
      return DirectoryIterator::Status::success;
    }

    DirectoryIterator it(path);
    const auto status = it.open();

    if (status != DirectoryIterator::Status::success) {
      return status;
    }

    while (true) {
      auto next_entry_res = it.next();
      if (!next_entry_res) {
        return next_entry_res.error;
      } else if (!next_entry_res.value) {
        break;
      }

      const auto& entry = next_entry_res.value.value();

      if (entry.name_size <= 2) {
        continue;
      }

      if (entry.type == DirectoryEntry::Type::regular_file) {
        builder.maybe_add_file(path, precedence, parent_package, entry);

      } else if (entry.type == DirectoryEntry::Type::directory) {
        if (entry.name[0] == '+') {
          //  Package directory
          auto package_status = traverse_package(builder, path, precedence, parent_package, entry);
          if (package_status != DirectoryIterator::Status::success) {
            return package_status;
          }

        } else if (entry.name[0] == '@' && !builder.is_within_class_directory) {
          //  Class directory
          auto class_status = traverse_class_directory(builder, path, precedence, parent_package, entry);
          if (class_status != DirectoryIterator::Status::success) {
            return class_status;
          }

        } else if (std::strcmp(entry.name, "private") == 0 && !builder.is_within_private_directory) {
          //  Private directory
          auto priv_status = traverse_private_directory(builder, path, precedence, parent_package, entry);
          if (priv_status != DirectoryIterator::Status::success) {
            return priv_status;
          }

        }
      }
    }

    return DirectoryIterator::Status::success;
  }

  DirectoryIterator::Status build_search_path(SearchPathBuilder& builder, const std::vector<FilePath>& directories) {
    for (int64_t i = 0; i < int64_t(directories.size()); i++) {
      auto status = build_one(builder, directories[i], i, std::string());
      if (status != DirectoryIterator::Status::success) {
        return status;
      }
    }
    return DirectoryIterator::Status::success;
  }
}

SearchPath::CandidateMap& SearchPath::require_private_candidate_map(const FilePath& parent) {
  auto it = private_candidates.find(parent);
  if (it == private_candidates.end()) {
    private_candidates[parent] = {};
  }
  return private_candidates.at(parent);
}

Optional<const SearchCandidate*> SearchPath::search_for(const std::string& name) const {
  return SearchPath::search_for(candidate_files, name);
}

Optional<const SearchCandidate*> SearchPath::search_for(const std::string& name,
                                                        const FilePath& from_directory) const {
  auto maybe_private_it = private_candidates.find(from_directory);
  if (maybe_private_it != private_candidates.end()) {
    //  The directory containing this `name` has a private directory, so see if `name` is a private
    //  function.
    auto maybe_private_file = SearchPath::search_for(maybe_private_it->second, name);
    if (maybe_private_file) {
      return maybe_private_file;
    }
  }

  return search_for(name);
}

Optional<const SearchCandidate*> SearchPath::search_for(const SearchPath::CandidateMap& map, const std::string& key) {
  auto it = map.find(key);
  return it == map.end() ? NullOpt{} : Optional<const SearchCandidate*>(&it->second);
}

int64_t SearchPath::size() const {
  return candidate_files.size() + private_candidates.size();
}

Optional<SearchPath> build_search_path_from_path_file(const FilePath& file) {
  std::ifstream ifs(file.c_str());
  if (!ifs) {
    return NullOpt{};
  }

  std::string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

  if (!mt::utf8::is_valid(contents)) {
    return NullOpt{};
  }

  const auto split_paths = split(contents, Character('\n'));
  std::vector<FilePath> directories;
  directories.reserve(split_paths.size());

  for (const auto& p : split_paths) {
    directories.emplace_back(std::string(p));
  }

  return build_search_path_from_paths(directories);
}

Optional<SearchPath> build_search_path_from_paths(const std::vector<FilePath>& directories) {
  SearchPath search_path;
  SearchPathBuilder builder(&search_path);

  auto res = build_search_path(builder, directories);

  if (res != DirectoryIterator::Status::success) {
    return NullOpt{};
  } else {
    return Optional<SearchPath>(std::move(search_path));
  }
}

}
