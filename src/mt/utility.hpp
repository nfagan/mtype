#pragma once

#include "config.hpp"
#include <utility>

#define MT_DELETE_COPY_CTOR_AND_ASSIGNMENT(class_name) \
  class_name(const class_name& other) = delete; \
  class_name& operator=(const class_name& other) = delete;

#define MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT(class_name) \
  class_name(class_name&& other) = default; \
  class_name& operator=(class_name&& other) = default;

#define MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(class_name) \
  class_name(class_name&& other) noexcept = default; \
  class_name& operator=(class_name&& other) noexcept = default;

#define MT_DEFAULT_COPY_CTOR_AND_ASSIGNMENT(class_name) \
  class_name(const class_name& other) = default; \
  class_name& operator=(const class_name& other) = default;

#define MT_NODISCARD [[nodiscard]]

namespace mt {

template <typename T, typename Stack>
struct ScopeHelper {
  template <typename... Args>
  ScopeHelper(T& apply_to, Args&&... args) : apply_to(apply_to) {
    Stack::push(apply_to, std::forward<Args>(args)...);
  }

  ~ScopeHelper() {
    Stack::pop(apply_to);
  }

  T& apply_to;
};

}