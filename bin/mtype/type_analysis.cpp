#include "type_analysis.hpp"

namespace mt {

ConcreteFunctionTypeInstance::ConcreteFunctionTypeInstance(const Library &library,
                                                           const Store &def_store,
                                                           const StringRegistry& string_registry) :
                                                           library(library),
                                                           def_store(def_store),
                                                           string_registry(string_registry) {
  //
}

namespace {
  std::string make_error_kind(const MatlabIdentifier& name,
                              const StringRegistry& string_registry) {
    return "type of parameter `" + string_registry.at(name.full_name()) + "`";
  }

  void check_parameters(ConcreteFunctionTypeInstance& instance,
                        const Type* source_type,
                        const FunctionParameters& params,
                        const MatlabScope* scope,
                        const Token* source_token,
                        const std::unordered_set<const Type*>& unbound_vars) {
    for (const auto& param : params) {
      if (param.is_ignored()) {
        continue;
      }

      auto maybe_var = scope->lookup_local_variable(param.name);
      if (!maybe_var.is_valid()) {
        continue;
      }

      auto maybe_type = instance.library.lookup_local_variable(maybe_var);
      if (!maybe_type) {
        continue;
      }

      if (unbound_vars.count(maybe_type.value()) > 0) {
        auto kind_str = make_error_kind(param.name, instance.string_registry);
        auto err =
          make_could_not_infer_type_error(source_token, std::move(kind_str), source_type);
        instance.errors.push_back(std::move(err));
      }
    }
  }
}

void check_for_concrete_function_type(ConcreteFunctionTypeInstance& instance,
                                      const FunctionDefHandle& def_handle,
                                      const Type* source_type) {
  const auto& library = instance.library;
  const auto& store = instance.def_store;

  IsFullyConcreteInstance concrete_instance;
  IsFullyConcrete check_fully_concrete{&concrete_instance};
  bool is_fully_concrete = source_type->accept(check_fully_concrete);

  if (is_fully_concrete) {
    return;
  }

  const auto& unbound_vars = concrete_instance.unbound_variables;

  store.use<Store::ReadConst>([&](const auto& reader) {
    const auto& def = reader.at(def_handle);
    const auto& header = def.header;
    const auto& scope = def.scope;
    const auto tok = header.name_token.get();

    check_parameters(instance, source_type, header.inputs, scope, tok, unbound_vars);
    check_parameters(instance, source_type, header.outputs, scope, tok, unbound_vars);
  });
}

}
