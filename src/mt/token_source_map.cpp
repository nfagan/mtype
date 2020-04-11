#include "token_source_map.hpp"
#include "Optional.hpp"

namespace mt {

/*
 * TokenSourceMap
 */

void TokenSourceMap::insert(const Token& tok, const TokenSourceMap::SourceData& data) {
  sources_by_token[tok] = data;
}

Optional<TokenSourceMap::SourceData> TokenSourceMap::lookup(const Token& tok) const {
  auto it = sources_by_token.find(tok);
  return it == sources_by_token.end() ? NullOpt{} : Optional<TokenSourceMap::SourceData>(it->second);
}

}
