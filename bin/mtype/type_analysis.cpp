#include "type_analysis.hpp"
#include "ast_store.hpp"

namespace mt {

ConcreteFunctionTypeInstance::ConcreteFunctionTypeInstance(const Library &library,
                                                           const Store &def_store,
                                                           const StringRegistry& string_registry) :
                                                           library(library),
                                                           def_store(def_store),
                                                           string_registry(string_registry) {
  //
}

using UntypedParameters =
  std::unordered_set<FunctionParameter, FunctionParameter::Hash>;

namespace {
  std::string make_error_kind(const UntypedParameters& params,
                              const StringRegistry& string_registry) {
    std::string base_str("type(s) of parameter(s) ");
    int64_t i = 0;
    for (const auto& param : params) {
      base_str += "`" + string_registry.at(param.name.full_name()) + "`";
      if (++i < int64_t(params.size())) {
        base_str += ", ";
      }
    }
    return base_str;
  }

  struct ExtractedFunctionInfo {
    const types::Abstraction* abstraction;
    const TypePtrs* inputs;
    const TypePtrs* outputs;
  };

  Optional<ExtractedFunctionInfo> extract_function_info(const Type* source) {
    const auto scheme_source = source->scheme_source();
    if (!scheme_source->is_abstraction()) {
      return NullOpt{};
    }

    const auto& abstr = MT_ABSTR_REF(*scheme_source);
    const TypePtrs* inputs = nullptr;
    const TypePtrs* outputs = nullptr;

    if (abstr.inputs->is_destructured_tuple()) {
      inputs = &MT_DT_REF(*abstr.inputs).members;
    }
    if (abstr.outputs->is_destructured_tuple()) {
      outputs = &MT_DT_REF(*abstr.outputs).members;
    }

    return Optional<ExtractedFunctionInfo>(ExtractedFunctionInfo{&abstr, inputs, outputs});
  }

  void add_untyped_parameters(UntypedParameters& untyped_params,
                              const FunctionParameters& params) {
    for (const auto& param : params) {
      if (!param.is_ignored()) {
        untyped_params.insert(param);
      }
    }
  }

  void check_parameters(const TypePtrs* param_types,
                        const FunctionParameters& params,
                        IsFullyConcrete& check_fully_concrete,
                        UntypedParameters& untyped_params) {
    if (param_types &&
        param_types->size() == params.size()) {
      for (int64_t i = 0; i < int64_t(param_types->size()); i++) {
        const auto& type = (*param_types)[i];
        const auto& param = params[i];

        if (!type->accept(check_fully_concrete) &&
            !param.is_ignored()) {
          untyped_params.insert(param);
        }
      }
    } else {
      add_untyped_parameters(untyped_params, params);
    }
  }
}

void check_for_concrete_function_type(ConcreteFunctionTypeInstance& instance,
                                      const FunctionDefHandle& def_handle,
                                      const Type* source_type) {
  const auto& store = instance.def_store;
  std::unordered_set<FunctionParameter, FunctionParameter::Hash> untyped_params;
  FunctionParameters inputs;
  FunctionParameters outputs;
  const Token* source_token;

  store.use<Store::ReadConst>([&](const auto& reader) {
    const auto& def = reader.at(def_handle);
    const auto& header = def.header;
    inputs = header.inputs;
    outputs = header.outputs;
    source_token = header.name_token.get();
  });

  auto maybe_function_info = extract_function_info(source_type);
  if (!maybe_function_info) {
    //  All parameters were untyped.
    add_untyped_parameters(untyped_params, inputs);
    add_untyped_parameters(untyped_params, outputs);

  } else {
    IsFullyConcreteInstance concrete_instance;
    concrete_instance.maybe_push_scheme_variables(source_type);
    IsFullyConcrete check_fully_concrete(&concrete_instance);

    const auto& function_info = maybe_function_info.value();
    check_parameters(function_info.inputs, inputs, check_fully_concrete, untyped_params);
    check_parameters(function_info.outputs, outputs, check_fully_concrete, untyped_params);
  }

  if (!untyped_params.empty()) {
    auto kind_str =
      make_error_kind(untyped_params, instance.string_registry);
    auto err =
      make_could_not_infer_type_error(source_token, std::move(kind_str), source_type);
    instance.errors.push_back(std::move(err));
  }
}

bool can_show_untyped_function_errors(const Store& store,
                                      const AstStore& ast_store,
                                      const FunctionDefHandle& def_handle) {
  //  Only show untyped function errors for files for which constraints
  //  were sucessfully generated.
  const CodeFileDescriptor* file_descriptor;

  store.use<Store::ReadConst>([&](const auto& reader) {
    file_descriptor = reader.at(def_handle).scope->file_descriptor;
  });

  const auto& file_path = file_descriptor->file_path;
  const auto maybe_entry = ast_store.lookup(file_path);

  return maybe_entry && maybe_entry->generated_type_constraints;
}

CheckRecursiveTypesInstance::CheckRecursiveTypesInstance(const PendingSchemes* pending_schemes) :
                                                         pending_schemes(pending_schemes) {
  //
}

void check_for_recursive_type(CheckRecursiveTypesInstance& instance,
                              const Type* type,
                              const Token* source_token) {
  IsRecursiveInstance::RemappedAliasSources remapped_alias_sources;

  for (const auto& pending_scheme : *instance.pending_schemes) {
    remapped_alias_sources[pending_scheme.target] =
      pending_scheme.scheme->scheme_source();
  }

  IsRecursiveInstance recursive_instance(&remapped_alias_sources);
  IsRecursive check_recursive(&recursive_instance);
  type->accept_const(check_recursive);

  if (recursive_instance.is_recursive) {
    auto err = make_recursive_type_error(source_token);
    instance.errors.push_back(std::move(err));
    instance.had_error = true;
  }
}

namespace {
  void check_recursive(CheckRecursiveTypesInstance& instance,
                       const TypeScope::TypeMap& type_map) {
    for (const auto& type_it : type_map) {
      const auto& type_ref = type_it.second;
      check_for_recursive_type(instance, type_ref->type, type_ref->source_token);
    }
  }
}

void check_for_recursive_types(CheckRecursiveTypesInstance& instance,
                               const TypeScope* root_scope) {
  check_recursive(instance, root_scope->local_types);
  check_recursive(instance, root_scope->exports);

  for (const auto& import : root_scope->imports) {
    check_recursive(instance, import.root->exports);
  }

  for (const auto& child : root_scope->children) {
    check_for_recursive_types(instance, child);
  }
}

}
