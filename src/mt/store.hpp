#pragma once

#include "ast.hpp"
#include "lang_components.hpp"
#include <mutex>
#include <thread>

#define MT_USE_SHARED_MUTEX (0)

#if MT_USE_SHARED_MUTEX
#include <shared_mutex>
#endif

namespace mt {

namespace detail {
#if MT_USE_SHARED_MUTEX
  using DefaultMutex = std::shared_mutex;
  template <typename T>
  using DefaultReadLock = std::shared_lock<T>;
#else
  using DefaultMutex = std::mutex;
  template <typename T>
  using DefaultReadLock = std::unique_lock<T>;
#endif

  template <typename T>
  using DefaultWriteLock = std::unique_lock<T>;

  template <typename T, typename Mutex = DefaultMutex, typename Lock = DefaultReadLock<Mutex>>
  class ReadCommon {
  public:
    explicit ReadCommon(T store) : store(store), lock(store.mutex) {
      //
    }

    template <typename... Args>
    auto& at(Args&&... args) {
      return store.at(std::forward<Args>(args)...);
    }

    template <typename... Args>
    const auto& at(Args&&... args) const {
      return store.at(std::forward<Args>(args)...);
    }

  public:
    T store;
    Lock lock;
  };
}

class VariableStore {
public:
  VariableStore() = default;
  ~VariableStore() = default;

  VariableDefHandle emplace_definition(VariableDef&& def);
  const VariableDef& at(const VariableDefHandle& handle) const;
  VariableDef& at(const VariableDefHandle& handle);

private:
  std::vector<VariableDef> definitions;
};

class FunctionStore {
private:
  template<typename T, typename M, typename L>
  friend class detail::ReadCommon;

  template <typename T>
  class Read : detail::ReadCommon<T> {
  public:
    using detail::ReadCommon<T>::ReadCommon;
    using detail::ReadCommon<T>::at;
  };

public:
  using ReadConst = Read<const FunctionStore&>;
  using ReadMut = Read<FunctionStore&>;

class Write : public detail::ReadCommon<FunctionStore&, detail::DefaultMutex,
  detail::DefaultWriteLock<detail::DefaultMutex>> {
  public:
    explicit Write(FunctionStore& store) : ReadCommon(store) {
      //
    }

    FunctionDefHandle emplace_definition(FunctionDef&& def) {
      return store.emplace_definition(std::forward<FunctionDef&&>(def));
    }
    template <typename... Args>
    auto make_external_reference(Args&&... args) {
      return store.make_external_reference(std::forward<Args>(args)...);
    }
    template <typename... Args>
    auto make_local_reference(Args&&... args) {
      return store.make_local_reference(std::forward<Args>(args)...);
    }
  };

public:
  FunctionStore() = default;
  ~FunctionStore() = default;

private:
  FunctionDefHandle emplace_definition(FunctionDef&& def);
  FunctionReferenceHandle make_external_reference(int64_t to_identifier, const MatlabScopeHandle& in_scope);
  FunctionReferenceHandle make_local_reference(int64_t to_identifier,
                                               const FunctionDefHandle& with_def,
                                               const MatlabScopeHandle& in_scope);

  const FunctionDef& at(const FunctionDefHandle& handle) const;
  FunctionDef& at(const FunctionDefHandle& handle);
  const FunctionReference& at(const FunctionReferenceHandle& handle) const;

private:
  mutable detail::DefaultMutex mutex;

  std::vector<FunctionDef> definitions;
  std::vector<FunctionReference> references;
};

class ClassStore {
private:
  template<typename T, typename M, typename L>
  friend class detail::ReadCommon;

  template <typename T>
  class Read : detail::ReadCommon<T> {
  public:
    using detail::ReadCommon<T>::ReadCommon;
    using detail::ReadCommon<T>::at;
  };
public:
  using ReadConst = Read<const ClassStore&>;
  using ReadMut = Read<ClassStore&>;

  class Write : public detail::ReadCommon<ClassStore&, detail::DefaultMutex,
    detail::DefaultWriteLock<detail::DefaultMutex>> {
  public:
    explicit Write(ClassStore& store) : ReadCommon(store) {
      //
    }
    ClassDefHandle make_definition() {
      return store.make_definition();
    }
    void emplace_definition(const ClassDefHandle& at_handle, ClassDef&& def) {
      store.emplace_definition(at_handle, std::forward<ClassDef&&>(def));
    }
  };

public:
  ClassStore() = default;
  ~ClassStore() = default;

private:
  ClassDefHandle make_definition();
  void emplace_definition(const ClassDefHandle& at_handle, ClassDef&& def);

  const ClassDef& at(const ClassDefHandle& by_handle) const;
  ClassDef& at(ClassDefHandle& by_handle);

private:
  mutable detail::DefaultMutex mutex;
  std::vector<ClassDef> definitions;
};

class ScopeStore {
private:
  template<typename T, typename M, typename L>
  friend class detail::ReadCommon;

  template <typename T>
  class Read : public detail::ReadCommon<T> {
  public:
    using detail::ReadCommon<T>::ReadCommon;
    using detail::ReadCommon<T>::at;
    using detail::ReadCommon<T>::store;

    template <typename... Args>
    auto lookup_local_function(Args&&... args) const {
      return store.lookup_local_function(std::forward<Args>(args)...);
    }
  };

public:
  using ReadConst = Read<const ScopeStore&>;
  using ReadMut = Read<ScopeStore&>;

class Write : public detail::ReadCommon<ScopeStore&, detail::DefaultMutex,
  detail::DefaultWriteLock<detail::DefaultMutex>> {
  public:
    explicit Write(ScopeStore& store) : ReadCommon(store) {
      //
    }
    template <typename... Args>
    auto make_matlab_scope(Args&&... args) {
      return store.make_matlab_scope(std::forward<Args>(args)...);
    }
  };

public:
  ScopeStore() = default;
  ~ScopeStore() = default;

private:
  FunctionReferenceHandle lookup_local_function(const MatlabScopeHandle& in_scope, int64_t name) const;
  MatlabScopeHandle make_matlab_scope(const MatlabScopeHandle& parent);

  const MatlabScope& at(const MatlabScopeHandle& handle) const;
  MatlabScope& at(const MatlabScopeHandle& handle);

private:
  mutable detail::DefaultMutex mutex;
  std::vector<MatlabScope> matlab_scopes;
};

}