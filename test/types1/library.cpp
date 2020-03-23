#include "library.hpp"
#include "type_store.hpp"
#include <cassert>

namespace mt {

bool Library::subtype_related(TypeRef lhs, TypeRef rhs) const {
  const auto& a = store.at(lhs);
  const auto& b = store.at(rhs);

  if (a.is_scalar() && b.is_scalar()) {
    return subtype_related(lhs, rhs, a.scalar, b.scalar);
  } else {
    return false;
  }
}

bool Library::subtype_related(TypeRef lhs, TypeRef rhs, const types::Scalar& a, const types::Scalar& b) const {
  if (a.identifier == b.identifier) {
    return true;
  }

  const auto& sub_it = scalar_subtype_relations.find(lhs);
  if (sub_it == scalar_subtype_relations.end()) {
    return false;
  }

  for (const auto& supertype : sub_it->second.supertypes) {
    if (supertype == rhs) {
      return true;
    }
  }

  return false;
}

Optional<TypeHandle> Library::lookup_function(const types::Abstraction& func) const {
  const auto func_it = function_types.find(func);
  return func_it == function_types.end() ? NullOpt{} : Optional<TypeHandle>(func_it->second);
}

bool Library::is_known_subscript_type(const TypeHandle& handle) const {
  return types_with_known_subscripts.count(handle) > 0;
}

Optional<std::string> Library::type_name(const TypeHandle& handle) const {
  const auto& type = store.at(handle);
  if (!type.is_scalar()) {
    return NullOpt{};
  }

  return type_name(type.scalar);
}

Optional<std::string> Library::type_name(const mt::types::Scalar& scl) const {
  const auto& name_it = scalar_type_names.find(scl.identifier);

  if (name_it == scalar_type_names.end()) {
    return NullOpt{};
  } else {
    return Optional<std::string>(string_registry.at(name_it->second));
  }
}

void Library::make_builtin_types() {
  double_type_handle = make_named_scalar_type("double");
  string_type_handle = make_named_scalar_type("string");
  char_type_handle = make_named_scalar_type("char");
  sub_double_type_handle = make_named_scalar_type("sub-double");

  //  Mark that sub-double is a subclass of double.
  scalar_subtype_relations[sub_double_type_handle] = types::SubtypeRelation(sub_double_type_handle, double_type_handle);
}

void Library::make_known_types() {
  make_builtin_types();
  make_subscript_references();
  make_binary_operators();
  make_free_functions();
  make_concatenations();
}

void Library::make_free_functions() {
  make_min();
  make_sum();
  make_fileparts();
  make_list_outputs_type();
  make_list_inputs_type();
  make_list_outputs_type2();
  make_sub_double();
}

void Library::make_sum() {
  using types::DestructuredTuple;
  using types::Abstraction;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();
  const auto& d_handle = double_type_handle;

  const auto ident = MatlabIdentifier(string_registry.register_string("sum"));

  Abstraction func(ident, args_type, result_type);
  auto func_copy = func;

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, d_handle)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, d_handle)));

  store.assign(func_type, Type(std::move(func)));
  function_types[func_copy] = func_type;
}

void Library::make_min() {
  using types::DestructuredTuple;
  using types::Abstraction;

  const auto args_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  const auto& double_handle = double_type_handle;
  const auto& char_handle = char_type_handle;
  const auto& str_handle = string_type_handle;
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

  const auto& double_handle = double_type_handle;
  const auto& char_handle = char_type_handle;

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
  const auto& double_handle = double_type_handle;
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

  const auto& double_handle = double_type_handle;
  const auto& char_handle = char_type_handle;
  const auto& string_handle = string_type_handle;

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

  const auto& double_handle = double_type_handle;
  const auto& char_handle = char_type_handle;
  const auto& string_handle = string_type_handle;

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
  make_builtin_parens_tuple_subscript_reference();
}

void Library::make_builtin_parens_subscript_references() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  std::vector<TypeHandle> default_array_types{double_type_handle, char_type_handle};

  for (const auto& referent_type_handle : default_array_types) {
    const auto args_type = store.make_type();
    const auto list_type = store.make_type();
    const auto result_type = store.make_type();
    const auto func_type = store.make_type();

    Abstraction ref(SubscriptMethod::parens, args_type, result_type);
    auto ref_copy = ref;

    std::vector<TypeHandle> list_types{double_type_handle};

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

void Library::make_builtin_parens_tuple_subscript_reference() {
  using types::DestructuredTuple;
  using types::Abstraction;
  using types::List;

  auto func_scheme = store.make_type();
  auto ref_var = store.make_fresh_type_variable_reference();
  auto tup_type = store.make_type();
  auto list_type = store.make_list(ref_var);

  types::Scheme scheme;

  //  {list<T>}
  store.assign(tup_type, Type(types::Tuple(list_type)));

  const auto args_type = store.make_type();
  const auto list_subs_type = store.make_type();
  const auto result_type = store.make_type();
  const auto func_type = store.make_type();

  types::Abstraction ref(SubscriptMethod::parens, args_type, result_type);
  auto ref_copy = ref;

  store.assign(list_subs_type, Type(List(double_type_handle)));  //  list<double>

  store.assign(args_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, tup_type, list_subs_type)));
  store.assign(result_type,
    Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, tup_type)));

  scheme.type = func_type;
  scheme.parameters.push_back(ref_var);

  store.assign(func_type, Type(std::move(ref)));
  store.assign(func_scheme, Type(std::move(scheme)));

  function_types[ref_copy] = func_scheme;
  types_with_known_subscripts.insert(tup_type);
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

  store.assign(list_subs_type, Type(List(double_type_handle)));  //  list<double>
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

    const auto& double_handle = double_type_handle;

    store.assign(args_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_inputs, double_handle, double_handle)));
    store.assign(result_type,
      Type(DestructuredTuple(DestructuredTuple::Usage::definition_outputs, double_handle)));
    store.assign(func_type, Type(std::move(abstr)));

    function_types[abstr_copy] = func_type;
  }
}

void Library::make_sub_double() {
  const auto name = MatlabIdentifier(string_registry.register_string("sub_double"));
  const auto args = store.make_output_destructured_tuple(TypeHandles());
  const auto outs = store.make_input_destructured_tuple(sub_double_type_handle);
  const auto func = store.make_abstraction(name, args, outs);

  types::Abstraction abstr_copy = store.at(func).abstraction;

  function_types[abstr_copy] = func;
}

TypeHandle Library::make_named_scalar_type(const char* name) {
  const auto name_id = string_registry.register_string(name);
  const auto type_handle = store.make_concrete();
  assert(store.type_of(type_handle) == Type::Tag::scalar);
  const auto& type = store.at(type_handle).scalar;

  scalar_type_names[type.identifier] = name_id;
  return type_handle;
}

}