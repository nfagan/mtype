#pragma once

#include "config.hpp"
#include <utility>
#include <memory>

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
struct StackHelper {
  template <typename... Args>
  StackHelper(T& apply_to, Args&&... args) : apply_to(apply_to) {
    Stack::push(apply_to, std::forward<Args>(args)...);
  }

  ~StackHelper() {
    Stack::pop(apply_to);
  }

  T& apply_to;
};

//  Via Andrei Alexandrescu, cpp con 2015.
namespace detail {
  enum class ScopeGuardOnExit {};
  template <typename Function>
  struct ScopeGuard {
    ScopeGuard(Function&& fun) : function(std::forward<Function>(fun)) {
      //
    }
    ~ScopeGuard() {
      function();
    }

    Function function;
  };

  template <typename Function>
  inline ScopeGuard<Function> operator+(ScopeGuardOnExit, Function&& fun) {
    return ScopeGuard<Function>(std::forward<Function>(fun));
  }
}

#define MT_CONCAT_IMPL(s1, s2) s1##s2
#define MT_CONCAT(s1, s2) MT_CONCAT_IMPL(s1, s2)
#ifdef __COUNTER__
#define MT_ANONYMOUS_VARIABLE(str) MT_CONCAT(str, __COUNTER__)
#else
#define MT_ANONYMOUS_VARIABLE(str) MT_CONCAT(str, __LINE__)
#endif

#define MT_SCOPE_EXIT \
  auto MT_ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = detail::ScopeGuardOnExit() + [&]()

template <typename T, typename U>
std::unique_ptr<U> downcast(std::unique_ptr<T> source) {
  static_assert(std::is_base_of_v<T, U>, "Cannot downcast from unrelated types.");
  auto derived = static_cast<U*>(source.release());
  return std::unique_ptr<U>(derived);
}

template <typename T, typename U>
std::unique_ptr<U> upcast(std::unique_ptr<T> source) {
  static_assert(std::is_base_of_v<U, T>, "Cannot upcast from unrelated types.");
  auto base = static_cast<U*>(source.release());
  return std::unique_ptr<U>(base);
}

}