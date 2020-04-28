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

/*
 * FunctionsByFile
 */

void FunctionsByFile::require(const CodeFileDescriptor* file_descriptor) {
  if (store.count(file_descriptor) == 0) {
    store[file_descriptor] = {};
  }
}

void FunctionsByFile::insert(const CodeFileDescriptor* file_descriptor,
                             const FunctionDefHandle& def_handle) {
  require(file_descriptor);
  store.at(file_descriptor).insert(def_handle);
}

}
