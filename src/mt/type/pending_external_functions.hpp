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

  explicit FunctionSearchResult(Type* type) : resolved_type(type) {
    //
  }

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
 * PendingFunction
 */

struct PendingFunction {
  struct Hash {
    std::size_t operator()(const PendingFunction& app) const noexcept;
  };
  friend inline bool operator==(const PendingFunction& a, const PendingFunction& b) {
    return a.function == b.function;
  }

  Type* function;
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

  using PendingFunctions =
    std::unordered_map<FunctionSearchCandidate,
                       std::unordered_set<PendingFunction, PendingFunction::Hash>,
                       CandidateHash>;

  bool has_resolved(const FunctionSearchCandidate& candidate) const;
  void add_resolved(const FunctionSearchCandidate& candidate, Type* with_type);

  void add_pending(const FunctionSearchCandidate& candidate,
                   const PendingFunction& app);
  void add_visited_candidate(const FunctionSearchCandidate& candidate);

  int64_t num_pending_candidate_files() const;

  VisitedCandidates visited_candidates;
  ResolvedCandidates resolved_candidates;
  PendingFunctions pending_functions;
};

}