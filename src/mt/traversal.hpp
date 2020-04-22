#pragma once

#include <vector>
#include "utility.hpp"

namespace mt {

class FunctionDefHandle;
class ClassDefHandle;
class MatlabIdentifier;
struct TypeIdentifier;

template <typename T>
class Optional;

namespace types {
  struct Class;
}

class TypeIdentifierNamespaceState {
  struct Stack {
    static void push(TypeIdentifierNamespaceState& state, const TypeIdentifier& ident);
    static void pop(TypeIdentifierNamespaceState& state);
  };
public:
  using Helper = StackHelper<TypeIdentifierNamespaceState, Stack>;

  void push(const TypeIdentifier& ident);
  void pop();
  MT_NODISCARD Optional<TypeIdentifier> enclosing_namespace() const;
  bool has_enclosing_namespace() const;
  void gather_components(std::vector<int64_t>& into) const;

private:
  std::vector<TypeIdentifier> components;
};

class TypeIdentifierExportState {
  struct Stack {
    static void push(TypeIdentifierExportState& state, bool value);
    static void pop(TypeIdentifierExportState& state);
  };
public:
  using Helper = StackHelper<TypeIdentifierExportState, Stack>;

  void push_export();
  void push_non_export();
  void pop_state();
  bool is_export() const;
  void dispatched_push(bool val);

private:
  std::vector<bool> state;
};

class AssignmentSourceState {
public:
  void push_assignment_target_rvalue();
  void push_non_assignment_target_rvalue();
  void pop_assignment_target_state();
  bool is_assignment_target_rvalue() const;

private:
  std::vector<bool> state;
};

class BooleanState {
  struct Stack {
    static void push(BooleanState& state, bool value);
    static void pop(BooleanState& state);
  };
public:
  using Helper = StackHelper<BooleanState, Stack>;

  void push(bool value);
  void pop();
  bool value() const;

private:
  std::vector<bool> state;
};

class ValueCategoryState {
public:
  void push_lhs();
  void push_rhs();
  void pop_side();

  bool is_lhs() const;

private:
  std::vector<bool> sides;
};

class FunctionDefState {
  struct Stack {
    static void push(FunctionDefState& state, const FunctionDefHandle& handle) {
      state.push_function(handle);
    }
    static void pop(FunctionDefState& state) {
      state.pop_function();
    }
  };

public:
  using Helper = StackHelper<FunctionDefState, Stack>;

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
  struct Stack {
    template <typename... Args>
    static void push(ClassDefState& state, Args&&... args) {
      state.push_class(std::forward<Args>(args)...);
    }
    static void pop(ClassDefState& state) {
      state.pop_class();
    }
  };

public:
  using Helper = StackHelper<ClassDefState, Stack>;

public:
  ClassDefState() = default;
  ~ClassDefState() = default;

  ClassDefHandle enclosing_class() const;
  types::Class* enclosing_class_type() const;
  MatlabIdentifier enclosing_class_name() const;
  MatlabIdentifier unqualified_enclosing_class_name() const;

  bool is_within_class() const;

  void push_default_state();

  void push_superclass_method_application();
  void push_non_superclass_method_application();
  void pop_superclass_method_application();

  bool is_within_superclass_method_application() const;

  void push_class(const ClassDefHandle& handle, const MatlabIdentifier& name);
  void push_class(const ClassDefHandle& handle, types::Class* type, const MatlabIdentifier& name);
  void push_class(const ClassDefHandle& handle, types::Class* type,
                  const MatlabIdentifier& full_name, const MatlabIdentifier& unqualified_name);
  void pop_class();

private:
  std::vector<bool> is_superclass_method_application;
  std::vector<ClassDefHandle> class_defs;
  std::vector<types::Class*> class_types;
  std::vector<MatlabIdentifier> class_names;
  std::vector<MatlabIdentifier> unqualified_class_names;
};

template <typename T>
class ScopeState {
  struct Stack {
    static void push(ScopeState<T>& state, T* scope) {
      state.push(scope);
    }
    static void pop(ScopeState<T>& state) {
      state.pop();
    }
  };
public:
  using Helper = StackHelper<ScopeState<T>, Stack>;

  void push(T* scope) {
    scopes.push_back(scope);
  }
  void pop() {
    scopes.pop_back();
  }
  T* current() const {
    return scopes.back();
  }

private:
  std::vector<T*> scopes;
};

}