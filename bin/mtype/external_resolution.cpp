#include "external_resolution.hpp"
#include "parse_pipeline.hpp"

namespace mt {

namespace {

struct HandleSearchResult {
  FunctionDefHandle def_handle;
  Token source_token;
};

Optional<HandleSearchResult> find_search_handle(const AstStore::Entry* entry,
                                                const FunctionSearchCandidate& candidate,
                                                const ParsePipelineInstanceData& instance_data) {
  const auto& store = instance_data.store;

  if (entry->file_entry_function_ref.is_valid()) {
    auto func_ref = store.get(entry->file_entry_function_ref);
    if (func_ref.name == candidate.function_name) {
      auto token = store.get_name_token(func_ref.def_handle);
      HandleSearchResult search_result{func_ref.def_handle, token};
      return Optional<HandleSearchResult>(search_result);
    }
  }

  if (entry->file_entry_class_def.is_valid()) {
    FunctionDefHandle maybe_def_handle;
    Token maybe_source_token;

    store.use<Store::ReadConst>([&](const auto& reader) {
      const auto& class_def = reader.at(entry->file_entry_class_def);

      for (const auto& method : class_def.methods) {
        //  @TODO: Needless linear search here.
        const auto& func_def = reader.at(method);
        const auto& attrs = func_def.attributes;
        const bool is_accessible = attrs.is_public() &&
          (attrs.is_constructor() || attrs.is_static());

        if (is_accessible &&
            func_def.header.name == candidate.function_name) {
          maybe_def_handle = method;
          maybe_source_token = func_def.header.name_token;
          return;
        }
      }
    });

    if (maybe_def_handle.is_valid()) {
      HandleSearchResult search_res{maybe_def_handle, maybe_source_token};
      return Optional<HandleSearchResult>(search_res);
    }
  }

  return NullOpt{};
}
}

std::size_t ResolutionPair::Hash::operator()(const ResolutionPair &pair) const noexcept {
  return std::hash<Type*>{}(pair.as_referenced) ^
         std::hash<Type*>{}(pair.as_defined);
}

ResolutionPairs
resolve_external_functions(ResolutionInstance& resolution_instance,
                           ParsePipelineInstanceData& instance_data,
                           PendingExternalFunctions& external_functions,
                           VisitedResolutionPairs& visited_pairs) {
  const auto& ast_store = instance_data.ast_store;
  const auto& store = instance_data.store;
  const auto& library = instance_data.library;

  ResolutionPairs result;

  for (auto& ext_candidate : external_functions.pending_applications) {
    const auto& candidate = ext_candidate.first;
    auto& pending_apps = ext_candidate.second;

    auto maybe_entry = ast_store.lookup(candidate.resolved_file->defining_file);
    if (!maybe_entry || !maybe_entry->root_block) {
      //  Didn't parse this file yet, or else failed to parse it.
      continue;
    }

    const auto look_for_type = find_search_handle(maybe_entry, candidate, instance_data);
    if (!look_for_type) {
      //  Error: could not locate function handle for this search candidate.
      for (const auto& pending_app : pending_apps) {
        auto err =
          make_unresolved_function_error(pending_app.source_token, pending_app.function);
        resolution_instance.errors.push_back(std::move(err));
      }
      pending_apps.clear();
      continue;
    }

    auto maybe_type = library.lookup_local_function(look_for_type.value().def_handle);
    if (!maybe_type) {
      //  We haven't yet registered the function's type, so defer.
      continue;
    }

    //  Mark that we discovered the type.
    auto type = maybe_type.value();
    external_functions.add_resolved(candidate, type);
    const auto& source_token = look_for_type.value().source_token;

    for (const auto& pending_app : pending_apps) {
      ResolutionPair resolution_pair{type, pending_app.function, source_token};

      if (visited_pairs.count(resolution_pair) == 0) {
        result.push_back(resolution_pair);
        visited_pairs.insert(resolution_pair);
      }
    }

    pending_apps.clear();
  }

  return result;
}

}
