#include "pre_imports.hpp"

namespace mt {

PreImport::PreImport(std::string identifier) :
  import_identifier(std::make_unique<std::string>(std::move(identifier))),
  anonymous_file_descriptor(std::make_unique<CodeFileDescriptor>(FilePath("<anonymous>"))),
  row_col_indices(std::make_unique<TextRowColumnIndices>()) {
  //
  row_col_indices->scan(import_identifier->c_str(), import_identifier->size());
}

Token PreImport::make_token() const {
  std::string_view lexeme = *import_identifier;
  const auto dummy_type = TokenType::identifier;
  return Token{dummy_type, lexeme};
}

ParseSourceData PreImport::to_parse_source_data() const {
  return ParseSourceData(*import_identifier,
                         anonymous_file_descriptor.get(),
                         row_col_indices.get());
}

PendingTypeImport PreImport::to_pending_type_import(StringRegistry& string_registry,
                                                    TypeScope* scope) const {

  const auto dummy_token = make_token();
  int64_t registered_identifier(string_registry.register_string(dummy_token.lexeme));
  const bool is_exported = false;

  return PendingTypeImport(dummy_token, registered_identifier, scope, is_exported);
}

}
