#pragma once

#include <cstdint>
#include <functional>

namespace mt {

namespace detail {
  template <int Disambiguator>
  class Handle {
  public:
    struct Hash {
      std::size_t operator()(const Handle& handle) const {
        return std::hash<int64_t>{}(handle.index);
      }
    };
    struct Less {
      bool operator()(const Handle& a, const Handle& b) const {
        return a.index < b.index;
      }
    };

  public:
    Handle() : index(-1) {
      //
    }

    bool is_valid() const {
      return index >= 0;
    }

    int64_t get_index() const {
      return index;
    }

    friend inline bool operator==(const Handle& a, const Handle& b) {
      return a.index == b.index;
    }
    friend inline bool operator!=(const Handle& a, const Handle& b) {
      return !(a == b);
    }
    friend inline bool operator<(const Handle& a, const Handle& b) {
      return a.index < b.index;
    }
    friend inline bool operator>(const Handle& a, const Handle& b) {
      return a.index > b.index;
    }

  protected:
    explicit Handle(int64_t index) : index(index) {
      //
    }

    int64_t index;
  };
}

class FunctionDefHandle : public detail::Handle<0> {
public:
  friend class Store;
  using Handle::Handle;
};

class FunctionReferenceHandle : public detail::Handle<1> {
public:
  friend class Store;
  using Handle::Handle;
};

class VariableDefHandle : public detail::Handle<2> {
public:
  friend class Store;
  using Handle::Handle;
};

class ClassDefHandle : public detail::Handle<3> {
public:
  friend class Store;
  using Handle::Handle;
};

}