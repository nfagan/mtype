#pragma once

#include "path.hpp"
#include "../Optional.hpp"
#include "../Result.hpp"

namespace mt {

namespace detail {
  struct PlatformHandle;
}

class DirectoryEntry {
  friend class DirectoryIterator;
public:
  enum class Type {
    regular_file,
    directory,
    unknown,
    invalid
  };
public:
  DirectoryEntry() : type(Type::invalid), name(nullptr), name_size(0) {
    //
  }
  DirectoryEntry(Type type, const char* name, int64_t name_size) :
  type(type), name(name), name_size(name_size) {
    //
  }

  ~DirectoryEntry() = default;

public:
  Type type;
  const char* name;
  int64_t name_size;

private:
  static DirectoryEntry from_platform_handle(detail::PlatformHandle* handle);
};

class DirectoryIterator {
public:
  enum class Status {
    success,
    error_already_open,
    error_not_yet_open,
    error_unspecified,
  };
public:
  explicit DirectoryIterator(const FilePath& directory_path);
  DirectoryIterator() = delete;

  DirectoryIterator(const DirectoryIterator& other) = delete;
  DirectoryIterator& operator=(const DirectoryIterator& other) = delete;
  DirectoryIterator(DirectoryIterator&& other) noexcept = delete;
  DirectoryIterator& operator=(DirectoryIterator&& other) noexcept = delete;

  ~DirectoryIterator();

  void close();
  [[nodiscard]] Status open();
  [[nodiscard]] Result<Status, Optional<DirectoryEntry>> next() const;

private:
  FilePath directory_path;
  bool is_open;
  detail::PlatformHandle* platform_handle;
};

}