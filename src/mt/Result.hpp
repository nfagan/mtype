#pragma once

#include <utility>

namespace mt {

template <typename E, typename V>
class Result {
public:
  Result() : is_error(false) {
    //
  }

  Result(const Result& other) : error(other.error), value(other.value), is_error(other.is_error) {
    //
  }
  Result(Result&& other) noexcept : error(std::move(other.error)), value(std::move(other.value)), is_error(other.is_error) {
    //
  }

  explicit Result(E&& err) : error(std::move(err)), is_error(true) {
    //
  }

  explicit Result(V&& value, int = 0) : value(std::move(value)), is_error(false) {
    //
  }

  Result& operator=(const Result& other) {
    if (this != &other) {
      is_error = other.is_error;
      value = other.value;
      error = other.error;
    }
    return *this;
  }

  Result& operator=(Result&& other) noexcept {
    if (this != &other) {
      is_error = other.is_error;
      value = std::move(other.value);
      error = std::move(other.error);
    }
    return *this;
  }

  // NOLINTNEXTLINE
  operator bool() const {
    return !is_error;
  }

  ~Result() = default;

  E error;
  V value;
  bool is_error;
};

template <typename E, typename V, typename... Args>
inline Result<E, V> make_error(Args&&... args) {
  E err(std::forward<Args...>(args...));
  return Result<E, V>(std::move(err));
}

template <typename E, typename V, typename... Args>
inline Result<E, V> make_success(Args&&... args) {
  V value(std::forward<Args...>(args...));
  return Result<E, V>(std::move(value), 1);
}

}