#pragma once

#include "mt/mt.hpp"

namespace mt {
  struct ParsePipelineInstanceData;

  struct ResolutionInstance {
    std::vector<BoxedTypeError> errors;
  };

  struct ResolutionPair {
    struct Hash {
      std::size_t operator()(const ResolutionPair& pair) const noexcept;
    };
    inline friend bool operator==(const ResolutionPair& a, const ResolutionPair& b) {
      return a.as_defined == b.as_defined && a.as_referenced == b.as_referenced;
    }

    Type* as_defined;
    Type* as_referenced;
    Token as_defined_source_token;
    const Token* as_referenced_source_token;
  };

  using ResolutionPairs = std::vector<ResolutionPair>;
  using VisitedResolutionPairs = std::unordered_set<ResolutionPair, ResolutionPair::Hash>;

  ResolutionPairs resolve_external_functions(ResolutionInstance& resolution_instance,
                                             ParsePipelineInstanceData& instance_data,
                                             PendingExternalFunctions& external_functions,
                                             VisitedResolutionPairs& visited_pairs);
}