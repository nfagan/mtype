#include "pending_external_functions.hpp"
#include "type_store.hpp"
#include "../fs/code_file.hpp"
#include "../search_path.hpp"
#include <functional>

namespace mt {

/*
 * FunctionSearchCandidate
 */

std::size_t
FunctionSearchCandidate::Hash::operator()(const FunctionSearchCandidate& candidate) const noexcept {
  const auto file_hash = std::hash<const SearchCandidate*>{}(candidate.resolved_file);
  const auto name_hash = MatlabIdentifier::Hash{}(candidate.function_name);
  return file_hash ^ name_hash;
}

FunctionSearchCandidate::FunctionSearchCandidate(const SearchCandidate* resolved_file,
                                                 const MatlabIdentifier& name) :
  resolved_file(resolved_file),
  function_name(name) {
  //
}

/*
 * PendingApplication
 */

std::size_t PendingFunction::Hash::operator()(const PendingFunction& app) const noexcept {
  return std::hash<Type*>{}(app.function);
}

/*
 * PendingExternalFunctions
 */

void PendingExternalFunctions::add_resolved(const FunctionSearchCandidate& candidate, Type* with_type) {
    resolved_candidates[candidate] = with_type;
}

bool PendingExternalFunctions::has_resolved(const FunctionSearchCandidate& candidate) const {
  return resolved_candidates.count(candidate) > 0;
}

void PendingExternalFunctions::add_pending(const FunctionSearchCandidate& candidate,
                                           const PendingFunction& app) {
  if (pending_applications.count(candidate) == 0) {
    pending_applications[candidate] = {};
  }

  auto& apps = pending_applications.at(candidate);
  apps.insert(app);
}

void PendingExternalFunctions::add_visited_candidate(const FunctionSearchCandidate& candidate) {
  visited_candidates.insert(candidate);
}

}
