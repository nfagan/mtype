#pragma once

#define MT_USE_READ_WRITE_OPTIM (0)

#include "handles.hpp"
#include "definitions.hpp"
#include "lang_components.hpp"
#include "type/type_scope.hpp"
#include <mutex>

#if MT_USE_READ_WRITE_OPTIM
#include <condition_variable>
#endif

namespace mt {

class CodeFileDescriptor;

namespace detail {
  using DefaultMutex = std::mutex;
}

class Store {
private:
  class StoreAccessor {
  public:
    template <typename T>
    class Read {
    public:
#if MT_USE_READ_WRITE_OPTIM
      explicit Read(T store) : store(store), accessor(store.accessor), lock(accessor.mutex) {
        accessor.write_condition.wait(lock, [&]() {
          return !accessor.is_writer_active && accessor.num_pending_writers == 0;
        });
        accessor.num_readers++;
        lock.unlock();
      }

      ~Read() {
        lock.lock();
        accessor.num_readers--;
        if (accessor.num_readers == 0) {
          lock.unlock();
          accessor.write_condition.notify_all();
        }
      }
#else
      explicit Read(T store) : store(store), accessor(store.accessor), lock(accessor.mutex) {
        //
      }
#endif

      template <typename... Args>
      const auto& at(Args&&... args) const {
        return store.at(std::forward<Args>(args)...);
      }
      template <typename... Args>
      auto& at(Args&&... args) {
        return store.at(std::forward<Args>(args)...);
      }

      template <typename... Args>
      auto lookup_local_function(Args&&... args) const {
        return store.lookup_local_function(std::forward<Args>(args)...);
      }

      template <typename... Args>
      auto extract_constructor(Args&&... args) const {
        return store.extract_constructor(std::forward<Args>(args)...);
      }

    private:
      T store;
      StoreAccessor& accessor;
      std::unique_lock<detail::DefaultMutex> lock;
    };

    class Write {
    public:
#if MT_USE_READ_WRITE_OPTIM
      explicit Write(Store& store) : store(store), accessor(store.accessor), lock(accessor.mutex) {
        accessor.num_pending_writers++;
        accessor.write_condition.wait(lock, [&]() {
          return accessor.num_readers == 0 && !accessor.is_writer_active;
        });
        accessor.num_pending_writers--;
        accessor.is_writer_active = true;
        lock.unlock();
      }

      ~Write() {
        lock.lock();
        accessor.is_writer_active = false;
        lock.unlock();
        accessor.write_condition.notify_all();
      }
#else
      explicit Write(Store& store) : store(store), accessor(store.accessor), lock(accessor.mutex) {
        //
      }
#endif

      template <typename... Args>
      auto emplace_definition(Args&&... args) {
        return store.emplace_definition(std::forward<Args>(args)...);
      }

      ClassDefHandle make_class_definition() {
        return store.make_class_definition();
      }

      template <typename... Args>
      FunctionDefHandle make_function_declaration(Args&&... args) {
        return store.make_function_declaration(std::forward<Args>(args)...);
      }

      template <typename... Args>
      auto make_matlab_scope(Args&&... args) {
        return store.make_matlab_scope(std::forward<Args>(args)...);
      }
      template <typename... Args>
      auto make_type_scope(Args&&... args) {
        return store.make_type_scope(std::forward<Args>(args)...);
      }
      template <typename... Args>
      auto make_external_reference(Args&&... args) {
        return store.make_external_reference(std::forward<Args>(args)...);
      }
      template <typename... Args>
      auto make_local_reference(Args&&... args) {
        return store.make_local_reference(std::forward<Args>(args)...);
      }
      template <typename... Args>
      const auto& at(Args&&... args) const {
        return store.at(std::forward<Args>(args)...);
      }
      template <typename... Args>
      auto& at(Args&&... args) {
        return store.at(std::forward<Args>(args)...);
      }

    private:
      Store& store;
      StoreAccessor& accessor;
      std::unique_lock<detail::DefaultMutex> lock;
    };

  public:
#if MT_USE_READ_WRITE_OPTIM
    StoreAccessor() : is_writer_active(false), num_pending_writers(0), num_readers(0) {
      //
    }
#else
    StoreAccessor() = default;
#endif

  private:
    mutable detail::DefaultMutex mutex;

#if MT_USE_READ_WRITE_OPTIM
    std::condition_variable write_condition;
    bool is_writer_active;
    int num_pending_writers;
    int num_readers;
#endif
  };

public:
  using ReadMut = StoreAccessor::Read<Store&>;
  using ReadConst = StoreAccessor::Read<const Store&>;
  using Write = StoreAccessor::Write;

  friend ReadMut;
  friend ReadConst;
  friend Write;

  template <typename T>
  using Callback = std::function<void(T&)>;
public:
  Store() = default;
  ~Store() = default;

  template <typename T>
  void use(const Callback<T>& cb) {
    T access(*this);
    cb(access);
  }

  template <typename T>
  void use(const Callback<T>& cb) const {
    T access(*this);
    cb(access);
  }

  FunctionReference get(const FunctionReferenceHandle& handle) const;
  Block* get_block(const FunctionDefHandle& handle) const;

private:
  MatlabScope* make_matlab_scope(const MatlabScope* parent, const CodeFileDescriptor* file_descriptor);
  TypeScope* make_type_scope(TypeScope* root, const TypeScope* parent);

  ClassDefHandle make_class_definition();
  FunctionDefHandle make_function_declaration(FunctionHeader&& header, const FunctionAttributes& attrs);

  VariableDefHandle emplace_definition(VariableDef&& def);
  FunctionDefHandle emplace_definition(FunctionDef&& def);
  void emplace_definition(const ClassDefHandle& at_handle, ClassDef&& def);

  FunctionReferenceHandle make_external_reference(const MatlabIdentifier& to_identifier,
                                                  const MatlabScope* in_scope);
  FunctionReferenceHandle make_local_reference(const MatlabIdentifier& to_identifier,
                                               const FunctionDefHandle& with_def,
                                               const MatlabScope* in_scope);

  const ClassDef& at(const ClassDefHandle& handle) const;
  ClassDef& at(const ClassDefHandle& handle);

  const VariableDef& at(const VariableDefHandle& handle) const;
  VariableDef& at(const VariableDefHandle& handle);

  const FunctionDef& at(const FunctionDefHandle& handle) const;
  FunctionDef& at(const FunctionDefHandle& handle);
  const FunctionReference& at(const FunctionReferenceHandle& handle) const;

  Optional<FunctionDefHandle> extract_constructor(const ClassDefHandle& for_class) const;

private:
  mutable StoreAccessor accessor;
  std::vector<VariableDef> variable_definitions;
  std::vector<ClassDef> class_definitions;
  std::vector<FunctionDef> function_definitions;
  std::vector<FunctionReference> function_references;
  std::vector<std::unique_ptr<MatlabScope>> matlab_scopes;
  std::vector<std::unique_ptr<TypeScope>> type_scopes;
};

}