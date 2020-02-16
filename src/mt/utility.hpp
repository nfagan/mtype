#pragma once

#ifdef _MSC_VER
#define MT_IS_MSVC (1)
#else
#define MT_IS_MSVC (0)
#endif

#ifndef NDEBUG
#define MT_DEBUG
#endif

#if MT_IS_MSVC
#define MSVC_MISSING_NOEXCEPT
#else
#define MSVC_MISSING_NOEXCEPT noexcept
#endif

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