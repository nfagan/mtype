#pragma once

#include "mt/mt.hpp"
#include <memory>
#include <string>

namespace mt {

class PreImport {
public:
  explicit PreImport(std::string identifier);

  MT_DEFAULT_MOVE_CTOR_AND_ASSIGNMENT_NOEXCEPT(PreImport)
  MT_DELETE_COPY_CTOR_AND_ASSIGNMENT(PreImport)

  MT_NODISCARD Token make_token() const;
  MT_NODISCARD ParseSourceData to_parse_source_data() const;
  MT_NODISCARD PendingTypeImport to_pending_type_import(StringRegistry& string_registry,
                                                        TypeScope* scope) const;

private:
  std::unique_ptr<std::string> import_identifier;
  std::unique_ptr<CodeFileDescriptor> anonymous_file_descriptor;
  std::unique_ptr<TextRowColumnIndices> row_col_indices;
};

using PreImports = std::vector<PreImport>;

}