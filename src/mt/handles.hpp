#pragma once

#include <cstdint>

namespace mt {

namespace detail {
  template <int Disambiguator>
  class Handle {
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

class MatlabScopeHandle : public detail::Handle<3> {
public:
  friend class Store;
  using Handle::Handle;
};

class ClassDefHandle : public detail::Handle<4> {
public:
  friend class Store;
  using Handle::Handle;
};

}