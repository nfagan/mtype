#pragma once

#include "../Optional.hpp"
#include "../identifier.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mt {

struct SearchCandidate;
struct Type;
class TypeStore;
struct Token;

namespace types {
  struct Application;
}

/*
 * FunctionSearchCandidate
 */

struct FunctionSearchCandidate {
  struct Hash {
    std::size_t operator()(const FunctionSearchCandidate& candidate) const noexcept;
  };

  FunctionSearchCandidate() : resolved_file(nullptr) {
    //
  }
  FunctionSearchCandidate(const SearchCandidate* resolved_file, const MatlabIdentifier& name);

  const SearchCandidate* resolved_file;
  MatlabIdentifier function_name;

  friend inline bool operator==(const FunctionSearchCandidate& a,
                                const FunctionSearchCandidate& b) {
    return a.resolved_file == b.resolved_file && a.function_name == b.function_name;
  }
  friend inline bool operator!=(const FunctionSearchCandidate& a,
                                const FunctionSearchCandidate& b) {
    return !(a == b);
  }
};

/*
 * FunctionSearchResult
 */

struct FunctionSearchResult {
  FunctionSearchResult() = default;

  FunctionSearchResult(Optional<Type*> type) : resolved_type(std::move(type)) {
    //
  }

  FunctionSearchResult(FunctionSearchCandidate&& candidate) :
    external_function_candidate(std::move(candidate)) {
    //
  }

  Optional<Type*> resolved_type;
  Optional<FunctionSearchCandidate> external_function_candidate;
};

/*
 * PendingApplication
 */

struct PendingApplication {
  struct Hash {
    std::size_t operator()(const PendingApplication& app) const noexcept;
  };
  friend inline bool operator==(const PendingApplication& a, const PendingApplication& b) {
    return a.application == b.application;
  }

  types::Application* application;
  const Token* source_token;
};

/*
 * PendingExternalFunctions
 */

struct PendingExternalFunctions {
  using CandidateHash = FunctionSearchCandidate::Hash;

  using VisitedCandidates =
    std::unordered_set<FunctionSearchCandidate, CandidateHash>;
  using ResolvedCandidates =
    std::unordered_map<FunctionSearchCandidate, Type*, CandidateHash>;

  using PendingApplications =
    std::unordered_map<FunctionSearchCandidate,
                       std::unordered_set<PendingApplication, PendingApplication::Hash>,
                       CandidateHash>;

  bool has_resolved(const FunctionSearchCandidate& candidate) const;
  void add_resolved(const FunctionSearchCandidate& candidate, Type* with_type);

  void add_application(const FunctionSearchCandidate& candidate,
                       const PendingApplication& app);
  void add_visited_candidate(const FunctionSearchCandidate& candidate);

  VisitedCandidates visited_candidates;
  ResolvedCandidates resolved_candidates;
  PendingApplications pending_applications;
};

}