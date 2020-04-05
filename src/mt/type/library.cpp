#include "library.hpp"
#include "type_store.hpp"
#include "../string.hpp"
#include "../fs/code_file.hpp"
#include "../fs/path.hpp"
#include <cassert>

namespace mt {

bool Library::subtype_related(const Type* lhs, const Type* rhs) const {
  if (lhs->is_scalar() && rhs->is_scalar()) {
    return subtype_related(lhs, rhs, MT_SCALAR_REF(*lhs), MT_SCALAR_REF(*rhs));
  } else {
    return false;
  }
}

bool Library::subtype_related(const Type* lhs, const Type* rhs, const types::Scalar& a, const types::Scalar& b) const {
  if (a.identifier == b.identifier) {
    return true;
  }

  const auto& sub_it = class_wrappers.find(lhs);
  if (sub_it == class_wrappers.end()) {
    return false;
  }

  for (const auto& supertype : sub_it->second->supertypes) {
    if (supertype == rhs || subtype_related(supertype, rhs)) {
      return true;
    }
  }

  return false;
}

Library::FunctionSearchResult Library::search_function(const types::Abstraction& func) const {
  const auto def_handle = maybe_extract_function_def(func);

  if (def_handle.is_valid()) {
    return lookup_local_function(def_handle);
  }

  //  `search_function` assumes `func`'s arguments are concrete. See unification.cpp.
  assert(func.inputs->is_destructured_tuple());

  auto maybe_method = method_dispatch(func, MT_DT_REF(*func.inputs).members);
  if (maybe_method) {
    return maybe_method;
  }

  if (func.is_function() && func.ref_handle.is_valid()) {
    auto ref = def_store.get(func.ref_handle);
    auto str_name = string_registry.at(ref.name.full_name());
    const auto& file_descriptor = *ref.scope->file_descriptor;

    Optional<const SearchPath::Candidate*> maybe_func_file;

    if (file_descriptor.represents_known_file()) {
      const auto containing_dir = fs::directory_name(file_descriptor.file_path);
      maybe_func_file = search_path.search_for(str_name, containing_dir);
    } else {
      maybe_func_file = search_path.search_for(str_name);
    }

    if (maybe_func_file) {
      return FunctionSearchResult(maybe_func_file);
    }
  }

  return lookup_pre_defined_external_function(func);
}

FunctionDefHandle Library::maybe_extract_function_def(const types::Abstraction& func) const {
  if (func.ref_handle.is_valid()) {
    return def_store.get(func.ref_handle).def_handle;
  } else {
    return FunctionDefHandle();
  }
}

Optional<Type*> Library::method_dispatch(const types::Abstraction& func, const TypePtrs& args) const {
  for (const auto& arg : args) {
    Type* arg_lookup = arg;

    if (arg_lookup->is_destructured_tuple()) {
      auto maybe_lookup = MT_DT_REF(*arg_lookup).first_non_destructured_tuple_member();
      if (!maybe_lookup) {
        //  Ill-formed search; arguments had an empty destructured tuple.
        return NullOpt{};
      }

      arg_lookup = maybe_lookup.value();
    }

    const auto maybe_class = class_for_type(arg_lookup);

    if (maybe_class) {
      const auto maybe_method = method_store.lookup_method(maybe_class.value(), func);

      if (maybe_method) {
        return Optional<Type*>(maybe_method.value());
      }
    }
  }

  return NullOpt{};
}

Optional<Type*> Library::lookup_function(const types::Abstraction& func) const {
  const auto def_handle = maybe_extract_function_def(func);

  if (def_handle.is_valid()) {
    return lookup_local_function(def_handle);
  } else {
    return lookup_pre_defined_external_function(func);
  }
}

Optional<Type*> Library::lookup_local_function(const FunctionDefHandle& def_handle) const {
  const auto func_it = local_function_types.find(def_handle);
  return func_it == local_function_types.end() ? NullOpt{} : Optional<Type*>(func_it->second);
}

Optional<Type*> Library::lookup_pre_defined_external_function(const types::Abstraction& func) const {
  const auto func_it = function_types.find(func);
  return func_it == function_types.end() ? NullOpt{} : Optional<Type*>(func_it->second);
}

void Library::emplace_local_function_type(const FunctionDefHandle& handle, Type* type) {
  local_function_types[handle] = type;
}

bool Library::is_known_subscript_type(const Type* handle) const {
  for (const auto& t : types_with_known_subscripts) {
    if (type_eq.related_entry(t, handle)) {
      return true;
    }
  }

  return false;
}

Optional<std::string> Library::type_name(const Type* type) const {
  if (!type->is_scalar()) {
    return NullOpt{};
  }

  return type_name(MT_SCALAR_REF(*type));
}

Optional<std::string> Library::type_name(const mt::types::Scalar& scl) const {
  const auto& name_it = scalar_type_names.find(scl.identifier);

  if (name_it == scalar_type_names.end()) {
    return NullOpt{};
  } else {
    return Optional<std::string>(string_registry.at(name_it->second));
  }
}

Optional<types::Class*> Library::class_wrapper(const Type* type) const {
  auto it = class_wrappers.find(type);
  if (it == class_wrappers.end()) {
    return NullOpt{};
  }
  return Optional<types::Class*>(it->second);
}

Optional<const types::Class*> Library::class_for_type(const Type* type) const {
  if (type->is_class()) {
    return Optional<const types::Class*>(MT_CLASS_PTR(type));
  } else {
    auto maybe_wrapper = class_wrapper(type);
    if (maybe_wrapper) {
      return Optional<const types::Class*>(maybe_wrapper.value());
    }
  }

  return NullOpt{};
}

void Library::make_builtin_types() {
  double_type_handle = make_named_scalar_type("double");
  string_type_handle = make_named_scalar_type("string");
  char_type_handle = make_named_scalar_type("char");
  sub_double_type_handle = make_named_scalar_type("sub-double");
  sub_sub_double_type_handle = make_named_scalar_type("sub-sub-double");
  logical_type_handle = make_named_scalar_type("logical");
  double_type_class = store.make_class(double_type_handle);

  auto sub_double_class_wrapper = store.make_class(sub_double_type_handle, double_type_handle);
  auto sub_sub_double_class_wrapper = store.make_class(sub_sub_double_type_handle, sub_double_type_handle);

  //  Mark that sub-double is a subclass of double.
  class_wrappers[sub_double_type_handle] = sub_double_class_wrapper;

  //  Mark that sub-sub-double is a subclass of sub-double.
  class_wrappers[sub_sub_double_type_handle] = sub_sub_double_class_wrapper;

  //  Add double class wrapper.
  class_wrappers[double_type_handle] = MT_CLASS_MUT_PTR(double_type_class);
}

void Library::make_known_types() {
  make_builtin_types();
  make_subscript_references();
  make_free_functions();
  make_concatenations();
  make_double_methods();
}

void Library::make_free_functions() {
  make_min();
  make_sum();
  make_list_outputs_type();
  make_list_inputs_type();
  make_list_outputs_type2();
  make_sub_sub_double();
  make_sub_double();
  make_double();
  make_function_as_input();
  make_feval();
  make_deal();
  make_logicals();
}

void Library::make_sum() {
  const auto ident = MatlabIdentifier(string_registry.register_string("sum"));
  const auto args_type = store.make_input_destructured_tuple(double_type_handle);
  const auto result_type = store.make_output_destructured_tuple(double_type_handle);
  const auto func_type = store.make_abstraction(ident, args_type, result_type);

  const auto func_copy = MT_ABSTR_REF(*func_type);
  function_types[func_copy] = func_type;
}

void Library::make_min() {
  const auto ident = MatlabIdentifier(string_registry.register_string("min"));
  const auto args_type = store.make_input_destructured_tuple(double_type_handle, string_type_handle);
  const auto result_type = store.make_output_destructured_tuple(double_type_handle, char_type_handle);
  const auto func_type = store.make_abstraction(ident, args_type, result_type);

  const auto func_copy = MT_ABSTR_REF(*func_type);
  function_types[func_copy] = func_type;
}

void Library::make_list_inputs_type() {
  const auto ident = MatlabIdentifier(string_registry.register_string("in_list"));
  const auto list_type = store.make_list(double_type_handle);
  const auto args_type = store.make_input_destructured_tuple(double_type_handle, list_type);
  const auto result_type = store.make_output_destructured_tuple(double_type_handle);
  const auto func_type = store.make_abstraction(ident, args_type, result_type);

  const auto func_copy = MT_ABSTR_REF(*func_type);
  function_types[func_copy] = func_type;
}

void Library::make_list_outputs_type() {
  const auto ident = MatlabIdentifier(string_registry.register_string("lists"));
  const auto list_type = store.make_list(TypePtrs{char_type_handle, double_type_handle});
  const auto args_type = store.make_input_destructured_tuple(double_type_handle);
  const auto result_type = store.make_output_destructured_tuple(double_type_handle, list_type);
  const auto func_type = store.make_abstraction(ident, args_type, result_type);

  const auto func_copy = MT_ABSTR_REF(*func_type);
  function_types[func_copy] = func_type;
}

void Library::make_list_outputs_type2() {
  const auto ident = MatlabIdentifier(string_registry.register_string("out_list"));
  const auto list_type = store.make_list(double_type_handle);
  const auto args_type = store.make_input_destructured_tuple(double_type_handle);
  const auto result_type = store.make_output_destructured_tuple(list_type);
  const auto func_type = store.make_abstraction(ident, args_type, result_type);

  const auto func_copy = MT_ABSTR_REF(*func_type);
  function_types[func_copy] = func_type;
}

void Library::make_subscript_references() {
  make_builtin_parens_subscript_references();
  make_builtin_brace_subscript_reference();
}

void Library::make_builtin_parens_subscript_references() {
  const auto ref_var = store.make_fresh_type_variable_reference();

  const auto list_subs_type = store.make_list(double_type_handle);
  const auto args_type = store.make_input_destructured_tuple(ref_var, list_subs_type);
  const auto result_type = store.make_output_destructured_tuple(ref_var);
  const auto func_type = store.make_abstraction(SubscriptMethod::parens, args_type, result_type);
  const auto ref_copy = MT_ABSTR_REF(*func_type);

  const auto scheme = store.make_scheme(func_type, TypePtrs{ref_var});
  function_types[ref_copy] = scheme;

  TypePtrs default_array_types{double_type_handle, char_type_handle,
                               string_type_handle, sub_double_type_handle};

  for (const auto& referent_type_handle : default_array_types) {
    add_type_with_known_subscript(referent_type_handle);
  }
}

void Library::make_builtin_brace_subscript_reference() {
  const auto ref_var = store.make_fresh_type_variable_reference();
  const auto tup_type = store.make_tuple(ref_var);

  const auto list_subs_type = store.make_list(double_type_handle);
  const auto args_type = store.make_input_destructured_tuple(tup_type, list_subs_type);
  const auto result_type = store.make_output_destructured_tuple(ref_var);
  const auto func_type = store.make_abstraction(SubscriptMethod::brace, args_type, result_type);
  const auto ref_copy = MT_ABSTR_REF(*func_type);

  const auto scheme = store.make_scheme(func_type, TypePtrs{ref_var});
  function_types[ref_copy] = scheme;
  add_type_with_known_subscript(tup_type);
}

void Library::make_concatenations() {
  const auto tvar = store.make_fresh_type_variable_reference();
  const auto list_args_type = store.make_list(tvar);
  const auto args_type = store.make_input_destructured_tuple(list_args_type);
  const auto result_type = store.make_output_destructured_tuple(tvar);
  const auto cat = store.make_abstraction(ConcatenationDirection::horizontal, args_type, result_type);
  const auto scheme = store.make_scheme(cat, TypePtrs{tvar});

  auto cat_copy = MT_ABSTR_REF(*cat);
  function_types[cat_copy] = scheme;
}

void Library::make_feval() {
  const auto name = MatlabIdentifier(string_registry.register_string("feval"));
  const auto arg_var = store.make_fresh_parameters();
  const auto result_var = store.make_fresh_type_variable_reference();
  const auto arg_func_type = store.make_abstraction(arg_var, result_var);

  const auto func_args = store.make_input_destructured_tuple(arg_func_type, arg_var);
  const auto func = store.make_abstraction(name, func_args, result_var);
  const auto func_scheme = store.make_scheme(func, TypePtrs{arg_var, result_var});

  const auto abstr_copy = MT_ABSTR_REF(*func);
  function_types[abstr_copy] = func_scheme;
}

void Library::make_deal() {
  const auto name = MatlabIdentifier(string_registry.register_string("deal"));
  const auto arg_var = store.make_fresh_parameters();

  const auto func_args = store.make_input_destructured_tuple(arg_var);
  const auto func_outs = store.make_output_destructured_tuple(arg_var);
  const auto func = store.make_abstraction(name, func_args, func_outs);
  const auto func_scheme = store.make_scheme(func, TypePtrs{arg_var});

  const auto abstr_copy = MT_ABSTR_REF(*func);
  function_types[abstr_copy] = func_scheme;
}

void Library::make_logicals() {
  auto tru = make_simple_function("true", {}, {logical_type_handle});
  auto fls = make_simple_function("false", {}, {logical_type_handle});

  const auto tru_copy = MT_ABSTR_REF(*tru);
  const auto fls_copy = MT_ABSTR_REF(*fls);

  function_types[tru_copy] = tru;
  function_types[fls_copy] = fls;
}

void Library::make_sub_double() {
  const auto name = MatlabIdentifier(string_registry.register_string("sub_double"));
  const auto args = store.make_input_destructured_tuple(TypePtrs());
  const auto outs = store.make_output_destructured_tuple(sub_double_type_handle);
  const auto func = store.make_abstraction(name, args, outs);

  types::Abstraction abstr_copy = MT_ABSTR_REF(*func);
  function_types[abstr_copy] = func;
}

void Library::make_sub_sub_double() {
  const auto name = MatlabIdentifier(string_registry.register_string("sub_sub_double"));
  const auto args = store.make_input_destructured_tuple(TypePtrs());
  const auto outs = store.make_output_destructured_tuple(sub_sub_double_type_handle);
  const auto func = store.make_abstraction(name, args, outs);

  types::Abstraction abstr_copy = MT_ABSTR_REF(*func);
  function_types[abstr_copy] = func;
}

void Library::make_double() {
  const auto name = MatlabIdentifier(string_registry.register_string("double"));
  const auto args = store.make_input_destructured_tuple(sub_double_type_handle);
  const auto outs = store.make_output_destructured_tuple(double_type_handle);
  const auto func = store.make_abstraction(name, args, outs);

  types::Abstraction abstr_copy = MT_ABSTR_REF(*func);
  function_types[abstr_copy] = func;
}

void Library::make_function_as_input() {
  const MatlabIdentifier name(string_registry.register_string("in_func"));
  const MatlabIdentifier in_name(string_registry.register_string("sum"));

  const auto input_args_type = store.make_input_destructured_tuple(double_type_handle);
  const auto output_args_type = store.make_output_destructured_tuple(double_type_handle);
  const auto input_func = store.make_abstraction(in_name, input_args_type, output_args_type);

  const auto input_args = store.make_input_destructured_tuple(input_func);
  const auto output_args = store.make_output_destructured_tuple(double_type_handle);
  const auto func = store.make_abstraction(name, input_args, output_args);

  types::Abstraction abstr_copy = MT_ABSTR_REF(*func);
  function_types[abstr_copy] = func;
}

void Library::make_double_methods() {
  std::vector<BinaryOperator> operators{BinaryOperator::plus, BinaryOperator::minus,
                                        BinaryOperator::times, BinaryOperator::matrix_times,
                                        BinaryOperator::right_divide, BinaryOperator::colon};

  TypePtrs arg_types{double_type_handle};

  for (const auto& op : operators) {
    for (const auto& arg : arg_types) {
      const auto args_type = store.make_input_destructured_tuple(arg, arg);
      const auto result_type = store.make_output_destructured_tuple(arg);
      const auto func_type = store.make_abstraction(op, args_type, result_type);
      const auto func_copy = MT_ABSTR_REF(*func_type);

      auto class_wrapper = class_for_type(arg);
      assert(class_wrapper);  //  wrapper should have already been created.
      method_store.add_method(class_wrapper.value(), func_copy, func_type);
    }
  }
}

Type* Library::make_named_scalar_type(const char* name) {
  const auto name_id = string_registry.register_string(name);
  const auto type_handle = store.make_concrete();
  assert(type_handle->is_scalar());
  const auto& type = MT_SCALAR_REF(*type_handle);

  scalar_type_names[type.identifier] = name_id;
  return type_handle;
}

Type* Library::make_simple_function(const char* name, TypePtrs&& args, TypePtrs&& outs) {
  auto input_tup = store.make_input_destructured_tuple(std::move(args));
  auto output_tup = store.make_output_destructured_tuple(std::move(outs));
  MatlabIdentifier name_ident(string_registry.register_string(name));

  return store.make_abstraction(name_ident, input_tup, output_tup);
}

void Library::add_type_with_known_subscript(const Type* t) {
  types_with_known_subscripts.push_back(t);
}

/*
 * MethodStore
 */

bool MethodStore::has_method(const types::Class* cls, const types::Abstraction& ref) const {
  if (methods.count(cls) == 0) {
    return false;
  }

  return methods.at(cls).count(ref) > 0;
}

Optional<Type*> MethodStore::lookup_method(const types::Class* cls, const types::Abstraction& by_header) const {
  if (methods.count(cls) == 0) {
    return NullOpt{};
  }

  const auto& methods_this_class = methods.at(cls);
  const auto& method_it = methods_this_class.find(by_header);

  if (method_it == methods_this_class.end()) {
    return NullOpt{};
  }

  return Optional<Type*>(method_it->second);
}

void MethodStore::add_method(const types::Class* to_class, const types::Abstraction& ref, Type* type) {
  auto& methods_this_class = require_methods(to_class);
  assert(methods_this_class.count(ref) == 0);
  methods_this_class[ref] = type;
}

MethodStore::TypedMethods& MethodStore::require_methods(const types::Class* for_class) {
  if (methods.count(for_class) == 0) {
    methods[for_class] = {};
  }
  return methods.at(for_class);
}

}