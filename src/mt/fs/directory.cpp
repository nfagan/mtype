#include "directory.hpp"
#include <dirent.h>
#include <cstring>

namespace mt {

namespace detail {
  struct PlatformHandle {
    PlatformHandle() : dir(nullptr), directory_entry(nullptr) {
      //
    }

    void close() {
      closedir(dir);
      dir = nullptr;
      directory_entry = nullptr;
    }

    DIR* dir;
    dirent* directory_entry;
  };
}

namespace {
DirectoryEntry::Type type_from_platform_directory_entry(const dirent* entry) {
  const auto type = entry->d_type;

  if (type == DT_REG) {
    return DirectoryEntry::Type::regular_file;

  } else if (type == DT_DIR) {
    return DirectoryEntry::Type::directory;

  } else {
    return DirectoryEntry::Type::unknown;
  }
}
}

DirectoryEntry DirectoryEntry::from_platform_handle(detail::PlatformHandle* handle) {
  const auto* platform_entry = handle->directory_entry;

  const auto type = type_from_platform_directory_entry(platform_entry);
  const char* name = platform_entry->d_name;
  const auto name_size = std::strlen(name);

  return DirectoryEntry(type, name, name_size);
}

DirectoryIterator::DirectoryIterator(const FilePath& directory_path) :
directory_path(directory_path), is_open(false), platform_handle(new detail::PlatformHandle()) {
  //
}

DirectoryIterator::~DirectoryIterator() {
  close();
  delete platform_handle;
  platform_handle = nullptr;
}

Result<DirectoryIterator::Status, Optional<DirectoryEntry>> DirectoryIterator::next() const {
  using Left = DirectoryIterator::Status;
  using Right = Optional<DirectoryEntry>;

  if (!is_open) {
    return make_error<Left, Right>(DirectoryIterator::Status::error_not_yet_open);
  }

  auto* platform_dir_entry = readdir(platform_handle->dir);

  if (platform_dir_entry == nullptr) {
    return make_success<Left, Right>(NullOpt{});

  } else {
    platform_handle->directory_entry = platform_dir_entry;
    const auto dir_entry = DirectoryEntry::from_platform_handle(platform_handle);
    return make_success<Left, Right>(Optional<DirectoryEntry>(dir_entry));

  }
}

DirectoryIterator::Status DirectoryIterator::open() {
  if (is_open) {
    return DirectoryIterator::Status::error_already_open;
  }

  if (platform_handle == nullptr) {
    platform_handle = new detail::PlatformHandle();
  }

  DIR* dir = opendir(directory_path.c_str());

  if (dir == nullptr) {
    return DirectoryIterator::Status::error_unspecified;
  }

  platform_handle->dir = dir;
  is_open = true;

  return DirectoryIterator::Status::success;
}

void DirectoryIterator::close() {
  if (is_open) {
    platform_handle->close();
  }
  is_open = false;
}

}