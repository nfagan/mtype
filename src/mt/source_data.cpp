#include "source_data.hpp"
#include "Optional.hpp"

namespace mt {

/*
 * TokenSourceMap
 */

void TokenSourceMap::insert(const Token& tok, const ParseSourceData& data) {
  sources_by_token[tok] = data;
}

Optional<ParseSourceData> TokenSourceMap::lookup(const Token& tok) const {
  auto it = sources_by_token.find(tok);
  return it == sources_by_token.end() ? NullOpt{} : Optional<ParseSourceData>(it->second);
}

}
