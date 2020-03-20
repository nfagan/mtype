#include "library.hpp"
#include "type_store.hpp"

namespace mt {

Optional<TypeHandle> Library::lookup_function(const types::Abstraction& func) const {
  const auto func_it = function_types.find(func);
  return func_it == function_types.end() ? NullOpt{} : Optional<TypeHandle>(func_it->second);
}

bool Library::is_known_subscript_type(const TypeHandle& handle) const {
  return types_with_known_subscripts.count(handle) > 0;
}

void Library::make_known_types() {
  make_subscript_references();
  make_binary_operators();
  make_free_functions();
  make_concatenations();
}

void Library::make_free_functions() {
  make_min();
  make_fileparts();
  make_list_outputs_type();
  make_list_inputs_type();
  make_list_outputs_type2();
}

void Library::make_min() {
  using types::DestructuredTuple;
  using types::Abstraction;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;
  const auto& str_handle = store.string_type_handle;
  const auto name = std::string("min");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle, str_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, double_handle, char_handle)));

  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Library::make_fileparts() {
  using types::DestructuredTuple;
  using types::Abstraction;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;

  const auto name = std::string("fileparts");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  std::vector<TypeHandle> outputs{char_handle, double_handle, double_handle};

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, char_handle, double_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, std::move(outputs))));

  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Library::make_list_inputs_type() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();
  const auto& double_handle = store.double_type_handle;
  const auto list_type = store.make_list(double_handle);

  const auto ident = MatlabIdentifier(string_registry.register_string("in_list"));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, TypeHandles{double_handle, list_type})));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, double_handle)));
  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Library::make_list_outputs_type() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();
  const auto list_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;
  const auto& string_handle = store.string_type_handle;

  const auto name = std::string("lists");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  std::vector<TypeHandle> outputs{double_handle, list_type};
  std::vector<TypeHandle> list_pattern{char_handle, double_handle};

  store.assign(list_type, Type(List(std::move(list_pattern))));
  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, std::move(outputs))));
  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Library::make_list_outputs_type2() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  const auto& double_handle = store.double_type_handle;
  const auto& char_handle = store.char_type_handle;
  const auto& string_handle = store.string_type_handle;

  const auto name = std::string("out_list");
  const auto ident = MatlabIdentifier(string_registry.register_string(name));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  auto list_type = store.make_list(double_handle);

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, list_type)));
  store.assign(func_type, Type(std::move(func)));

  function_types[func_copy] = func_type;
}

void Library::make_subscript_references() {
  make_builtin_parens_subscript_references();
  make_builtin_brace_subscript_reference();
}

void Library::make_builtin_parens_subscript_references() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  std::vector<TypeHandle> default_array_types{store.double_type_handle, store.char_type_handle};

  for (const auto& referent_type_handle : default_array_types) {
    const auto args_type = store.make_type();
    const auto list_type = store.make_type();
    const auto result_type = store.make_type();
    const auto func_type = store.make_type();

    Abstraction ref(SubscriptMethod::parens, args_type, result_type);
    auto ref_copy = ref;

    std::vector<TypeHandle> list_types{store.double_type_handle};

    //  T(list<double>) -> T
    store.assign(list_type, Type(List(std::move(list_types))));
    store.assign(args_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, referent_type_handle, list_type)));
    store.assign(result_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, referent_type_handle)));

    store.assign(func_type, Type(std::move(ref)));

    function_types[ref_copy] = func_type;
    types_with_known_subscripts.insert(referent_type_handle);
  }
}

void Library::make_builtin_brace_subscript_reference() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  auto func_scheme = store.make_type();
  auto ref_var = store.make_fresh_type_variable_reference();
  auto tup_type = store.make_type();

  types::Scheme scheme;

  //  {T}
  store.assign(tup_type, Type(types::Tuple(ref_var)));

  const auto args_type = store.make_type();
  const auto list_subs_type = store.make_type();
  const auto list_result_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  types::Abstraction ref(SubscriptMethod::brace, args_type, result_type);
  auto ref_copy = ref;

  store.assign(list_subs_type, Type(List(store.double_type_handle)));  //  list<double>
  store.assign(list_result_type, Type(List(ref_var)));  //  list<T>

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, tup_type, list_subs_type)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, list_result_type)));

  scheme.type = func_type;
  scheme.parameters.push_back(ref_var);

  store.assign(func_type, Type(std::move(ref)));
  store.assign(func_scheme, Type(std::move(scheme)));

  function_types[ref_copy] = func_scheme;
  types_with_known_subscripts.insert(tup_type);
}

void Library::make_concatenations() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  auto func_scheme = store.make_type();
  auto tvar = store.make_fresh_type_variable_reference(); //  T

  types::Scheme scheme;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  types::Abstraction cat(ConcatenationDirection::horizontal, args_type, result_type);
  auto cat_copy = cat;

  auto list_args_type = store.make_list(tvar);  //  list<T>

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, list_args_type)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, tvar)));

  scheme.type = func_type;
  scheme.parameters.push_back(tvar);

  store.assign(func_type, Type(std::move(cat)));
  store.assign(func_scheme, Type(std::move(scheme)));

  function_types[cat_copy] = func_scheme;
}

void Library::make_binary_operators() {
  using types::DestructuredTuple;
  using types::Abstraction;

  std::vector<BinaryOperator> operators{BinaryOperator::plus, BinaryOperator::minus,
                                        BinaryOperator::times, BinaryOperator::matrix_times,
                                        BinaryOperator::right_divide, BinaryOperator::colon};

  for (const auto& op : operators) {
    auto args_type = store.make_type();
    auto result_type = store.make_type();
    auto func_type = store.make_type();

    Abstraction abstr(op, args_type, result_type);
    Abstraction abstr_copy = abstr;

    const auto& double_handle = store.double_type_handle;

    store.assign(args_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle, double_handle)));
    store.assign(result_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, double_handle)));
    store.assign(func_type, Type(std::move(abstr)));

    function_types[abstr_copy] = func_type;
  }
}

}