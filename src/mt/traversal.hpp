#pragma once

#include <vector>
#include "utility.hpp"

namespace mt {

class FunctionDefHandle;
class ClassDefHandle;

class FunctionDefState {
  struct EnclosingFunctionStack {
    static void push(FunctionDefState& state, const FunctionDefHandle& handle) {
      state.push_function(handle);
    }
    static void pop(FunctionDefState& state) {
      state.pop_function();
    }
  };

public:
  using EnclosingFunctionHelper = ScopeHelper<FunctionDefState, EnclosingFunctionStack>;

public:
  FunctionDefState() = default;
  ~FunctionDefState() = default;

  FunctionDefHandle enclosing_function() const;

private:
  void push_function(const FunctionDefHandle& func);
  void pop_function();

private:
  std::vector<FunctionDefHandle> function_defs;
};

class ClassDefState {
  struct EnclosingClassStack {
    static void push(ClassDefState& state, const ClassDefHandle& handle) {
      state.push_class(handle);
    }
    static void pop(ClassDefState& state) {
      state.pop_class();
    }
  };

public:
  using EnclosingClassHelper = ScopeHelper<ClassDefState, EnclosingClassStack>;

public:
  ClassDefState() = default;
  ~ClassDefState() = default;

  ClassDefHandle enclosing_class() const;

  void push_default_state();

  void push_superclass_method_application();
  void push_non_superclass_method_application();
  void pop_superclass_method_application();

  bool is_within_superclass_method_application() const;

private:
  void push_class(const ClassDefHandle& handle);
  void pop_class();

private:
  std::vector<bool> is_superclass_method_application;
  std::vector<ClassDefHandle> class_defs;
};

}