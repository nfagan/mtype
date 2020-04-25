#include "library.hpp"
#include "type_store.hpp"
#include "../search_path.hpp"
#include "../string.hpp"
#include "../fs/code_file.hpp"
#include "../fs/path.hpp"
#include <cassert>

namespace mt {

namespace {
  inline bool class_eq(const types::Class* lhs, const types::Class* rhs) {
    return lhs->name == rhs->name;
  }
}

bool Library::subtype_related(const Type* lhs, const Type* rhs) const {
  auto maybe_lhs_cls = class_for_type(lhs);
  auto maybe_rhs_cls = class_for_type(rhs);

  if (!maybe_lhs_cls || !maybe_rhs_cls) {
    return false;
  } else if (class_eq(maybe_lhs_cls.value(), maybe_rhs_cls.value())) {
    return true;
  }

  const auto* lhs_cls = maybe_lhs_cls.value();
  const auto* rhs_cls = maybe_rhs_cls.value();

  for (const auto& supertype : lhs_cls->supertypes) {
    if (subtype_related(supertype, rhs_cls)) {
      return true;
    }
  }

  return false;
}

Library::FunctionSearchResult Library::search_function(const types::Abstraction& func,
                                                       const TypePtrs& args) const {
  const auto def_handle = maybe_extract_function_def(func);

  if (def_handle.is_valid()) {
    return lookup_local_function(def_handle);
  }

  auto maybe_method = method_dispatch(func, args);
  if (maybe_method) {
    return maybe_method;
  }

  if (func.is_function() && func.ref_handle.is_valid()) {
    auto ref = def_store.get(func.ref_handle);
    auto str_name = string_registry.at(ref.name.full_name());
    const auto& file_descriptor = *ref.scope->file_descriptor;

    Optional<const SearchCandidate*> maybe_func_file;

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
    const auto maybe_arg_lookup = types::DestructuredTuple::type_or_first_non_destructured_tuple_member(arg);
    if (!maybe_arg_lookup) {
      return NullOpt{};
    }

    const auto maybe_method = lookup_method(maybe_arg_lookup.value(), func);
    if (maybe_method) {
      return maybe_method;
    }
  }

  return NullOpt{};
}

Optional<Type*> Library::lookup_method(const Type* type, const types::Abstraction& func) const {
  const auto maybe_class = class_for_type(type);

  if (maybe_class) {
    const auto maybe_method = method_store.lookup_method(maybe_class.value(), func);
    if (maybe_method) {
      return Optional<Type*>(maybe_method.value());
    }

    for (const auto& supertype : maybe_class.value()->supertypes) {
      auto maybe_super_method = lookup_method(supertype, func);
      if (maybe_super_method) {
        return maybe_super_method;
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

Optional<types::Class*> Library::lookup_local_class(const ClassDefHandle& def_handle) const {
  assert(def_handle.is_valid());
  const auto class_it = local_class_types.find(def_handle);
  return class_it == local_class_types.end() ? NullOpt{} : Optional<types::Class*>(class_it->second);
}

Optional<Type*> Library::lookup_local_variable(const VariableDefHandle& def_handle) const {
  const auto var_it = local_variables_types.find(def_handle);
  return var_it == local_variables_types.end() ? NullOpt{} : Optional<Type*>(var_it->second);
}

Optional<Type*> Library::lookup_pre_defined_external_function(const types::Abstraction& func) const {
  const auto func_it = function_types.find(func);
  return func_it == function_types.end() ? NullOpt{} : Optional<Type*>(func_it->second);
}

void Library::emplace_local_function_type(const FunctionDefHandle& handle, Type* type) {
  assert(local_function_types.count(handle) == 0);
  local_function_types[handle] = type;
}

void Library::emplace_local_class_type(const ClassDefHandle& handle, types::Class* type) {
  assert(local_class_types.count(handle) == 0);
  local_class_types[handle] = type;
}

void Library::emplace_local_variable_type(const VariableDefHandle& handle, Type* type) {
//  assert(local_variables_types.count(handle) == 0);
  local_variables_types[handle] = type;
}

Type* Library::require_local_variable_type(const VariableDefHandle& handle) {
  auto it = local_variables_types.find(handle);
  if (it == local_variables_types.end()) {
    auto var = store.make_fresh_type_variable_reference();
    local_variables_types[handle] = var;
    return var;
  } else {
    return it->second;
  }
}

bool Library::is_known_subscript_type(const Type* handle) const {
  for (const auto& t : types_with_known_subscripts) {
    if (type_eq.related_entry(t, handle)) {
      return true;
    }
  }

  return false;
}

bool Library::is_builtin_class(const MatlabIdentifier& ident) const {
  return ident == MatlabIdentifier(special_identifiers.handle);
}

bool Library::has_declared_function_type(const TypeIdentifier& ident) const {
  return declared_function_types.count(ident) > 0;
}

void Library::register_declared_function(const TypeIdentifier& ident) {
  declared_function_types.insert(ident);
}

bool Library::emplace_declared_function_type(const types::Abstraction& abstr, Type* source) {
  if (function_types.count(abstr) > 0) {
    return false;
  } else {
    function_types[abstr] = source;
    return true;
  }
}

Optional<std::string> Library::type_name(const Type* type) const {
  if (!type->is_scalar()) {
    return NullOpt{};
  }

  const auto& scl = MT_SCALAR_REF(*type);
  return string_registry.maybe_at(scl.identifier.full_name());
}

Optional<types::Class*> Library::lookup_class(const TypeIdentifier& name) const {
  auto it = class_types.find(name);
  return it == class_types.end() ? NullOpt{} : Optional<types::Class*>(it->second);
}

Optional<types::Class*> Library::class_wrapper(const Type* type) const {
  if (!type->is_scalar()) {
    return NullOpt{};
  }

  const auto& scl = MT_SCALAR_REF(*type);
  return lookup_class(scl.identifier);
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
  auto double_t = make_named_scalar_type("double");
  auto string_t = make_named_scalar_type("string");
  auto char_t = make_named_scalar_type("char");
  auto log_t = make_named_scalar_type("logical");
  //  For debugging subtype relations.
  auto sub_double_t = make_named_scalar_type("sub-double");

  double_id = double_t->identifier;
  string_id = string_t->identifier;
  char_id = char_t->identifier;
  logical_id = log_t->identifier;
  sub_double_id = sub_double_t->identifier;

  add_type_to_base_scope(double_id, double_t);
  add_type_to_base_scope(string_id, string_t);
  add_type_to_base_scope(char_id, char_t);
  add_type_to_base_scope(logical_id, log_t);

  auto sub_wrapper = make_class_wrapper(sub_double_id, sub_double_t);
  sub_wrapper->supertypes.push_back(double_t);
}

void Library::make_known_types() {
  make_base_type_scope();
  make_builtin_types();
  make_subscript_references();
  make_free_functions();
  make_concatenations();
  make_subtype_debug();
}

void Library::make_base_type_scope() {
  def_store.use<Store::Write>([&](auto& writer) {
    base_scope = writer.make_type_scope(nullptr, nullptr);
    base_scope->root = base_scope;
  });
}

void Library::make_free_functions() {
  make_feval();
  make_deal();
  make_logicals();
}

void Library::make_subscript_references() {
  make_builtin_parens_subscript_references();
  make_builtin_brace_subscript_reference();
}

void Library::make_builtin_parens_subscript_references() {
  auto maybe_number = get_number_type();
  auto maybe_char = get_char_type();
  auto maybe_str = get_string_type();

  if (!maybe_number || !maybe_char || !maybe_str) {
    return;
  }

  const auto ref_var = store.make_fresh_type_variable_reference();

  const auto list_subs_type = store.make_list(maybe_number.value());
  const auto args_type = store.make_input_destructured_tuple(ref_var, list_subs_type);
  const auto result_type = store.make_output_destructured_tuple(ref_var);
  const auto func_type = store.make_abstraction(SubscriptMethod::parens, args_type, result_type);
  const auto ref_copy = MT_ABSTR_REF(*func_type);

  const auto scheme = store.make_scheme(func_type, TypePtrs{ref_var});
  function_types[ref_copy] = scheme;

  TypePtrs default_array_types{maybe_number.value(), maybe_char.value(),
                               maybe_str.value()};

  for (const auto& referent_type_handle : default_array_types) {
    add_type_with_known_subscript(referent_type_handle);
  }
}

void Library::make_builtin_brace_subscript_reference() {
  auto maybe_number = get_number_type();
  if (!maybe_number) {
    return;
  }

  const auto ref_var = store.make_fresh_type_variable_reference();
  const auto tup_type = store.make_tuple(ref_var);

  const auto list_subs_type = store.make_list(maybe_number.value());
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
  auto maybe_logical = get_logical_type();
  if (!maybe_logical) {
    return;
  }

  auto tru = make_simple_function("true", {}, {maybe_logical.value()});
  auto fls = make_simple_function("false", {}, {maybe_logical.value()});

  const auto tru_copy = MT_ABSTR_REF(*tru);
  const auto fls_copy = MT_ABSTR_REF(*fls);

  function_types[tru_copy] = tru;
  function_types[fls_copy] = fls;
}

void Library::make_subtype_debug() {
  auto rec0 = store.make_record();
  auto f0_name = store.make_constant_value(TypeIdentifier(string_registry.register_string("x")));
  auto f0 = types::Record::Field{f0_name, get_number_type().value()};
  rec0->fields.push_back(f0);
  auto cls0 = store.make_class(TypeIdentifier(string_registry.register_string("cls0")), rec0);
  auto method0 = make_simple_function("method1", TypePtrs{cls0}, TypePtrs{get_number_type().value()});
  class_types[cls0->name] = cls0;
  method_store.add_method(cls0, *method0, method0);

  auto rec1 = store.make_record(*rec0);
  auto cls1 = store.make_class(TypeIdentifier(string_registry.register_string("cls1")), rec1);
  cls1->supertypes.push_back(cls0);
  class_types[cls1->name] = cls1;

  auto make_cls0 = make_simple_function("make_cls0", TypePtrs{}, TypePtrs{cls0});
  auto make_cls1 = make_simple_function("make_cls1", TypePtrs{}, TypePtrs{cls1});

  function_types[*make_cls0] = make_cls0;
  function_types[*make_cls1] = make_cls1;
}

types::Abstraction* Library::make_simple_function(const char* name, TypePtrs&& args, TypePtrs&& outs) {
  auto input_tup = store.make_input_destructured_tuple(std::move(args));
  auto output_tup = store.make_output_destructured_tuple(std::move(outs));
  MatlabIdentifier name_ident(string_registry.register_string(name));

  return store.make_abstraction(name_ident, input_tup, output_tup);
}

types::Class* Library::make_class_wrapper(const TypeIdentifier& name, Type* source) {
  auto cls = store.make_class(name, source);
  class_types[name] = cls;
  return cls;
}

types::Scalar* Library::make_named_scalar_type(const char* name) {
  auto type = scalar_store.make_named_scalar_type(name);
  process_scalar_type(type);
  return type;
}

types::Scalar* Library::make_named_scalar_type(const TypeIdentifier& name) {
  auto type = scalar_store.make_named_scalar_type(name);
  process_scalar_type(type);
  return type;
}

void Library::process_scalar_type(types::Scalar* type) {
  (void) make_class_wrapper(type->identifier, type);
}

void Library::add_type_to_base_scope(const TypeIdentifier& ident, Type* type) {
  auto ref = store.make_type_reference(nullptr, type, base_scope);
  base_scope->emplace_type(ident, ref, true);
}

void Library::add_type_with_known_subscript(const Type* t) {
  types_with_known_subscripts.push_back(t);
}

Optional<Type*> Library::get_number_type() const {
  return scalar_store.lookup(double_id);
}

Optional<Type*> Library::get_sub_number_type() const {
  return scalar_store.lookup(sub_double_id);
}

Optional<Type*> Library::get_char_type() const {
  return scalar_store.lookup(char_id);
}

Optional<Type*> Library::get_string_type() const {
  return scalar_store.lookup(string_id);
}

Optional<Type*> Library::get_logical_type() const {
  return scalar_store.lookup(logical_id);
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

bool MethodStore::has_named_method(const types::Class* cls, const MatlabIdentifier& name) const {
  types::Abstraction search_for(name, nullptr, nullptr);
  return has_method(cls, search_for);
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

/*
 * ScalarTypeStore
 */

types::Scalar* ScalarTypeStore::make_named_scalar_type(const char* name) {
  const auto identifier = TypeIdentifier(string_registry.register_string(name));
  return make_named_scalar_type(identifier);
}

types::Scalar* ScalarTypeStore::make_named_scalar_type(const TypeIdentifier& name) {
  const auto type = store.make_scalar(name);
  assert(type->is_scalar());
  scalar_types[name] = type;
  return MT_SCALAR_MUT_PTR(type);
}

bool ScalarTypeStore::contains(const TypeIdentifier& name) const {
  return scalar_types.count(name) > 0;
}

Optional<Type*> ScalarTypeStore::lookup(const TypeIdentifier& name) const {
  const auto it = scalar_types.find(name);
  return it == scalar_types.end() ? NullOpt{} : Optional<Type*>(it->second);
}

/*
 * SpecialIdentifierStore
 */

SpecialIdentifierStore::SpecialIdentifierStore(StringRegistry& string_registry) :
  subsref(string_registry.register_string("subsref")),
  subsindex(string_registry.register_string("subsindex")),
  identifier_struct(string_registry.register_string("struct")),
  handle(string_registry.register_string("handle")),
  varargin(string_registry.register_string("varargin")),
  varargout(string_registry.register_string("varargout")) {
  //
}

}