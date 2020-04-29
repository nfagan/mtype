#include "show.hpp"
#include "command_line.hpp"
#include "ast_store.hpp"

namespace mt {

namespace {
  inline const char* maybe_style(const char* code, bool rich_text) {
    return rich_text ? code : "";
  }
}

void configure_type_to_string(TypeToString& type_to_string, const cmd::Arguments& args) {
  type_to_string.explicit_destructured_tuples = args.show_explicit_destructured_tuples;
  type_to_string.explicit_aliases = args.show_explicit_aliases;
  type_to_string.arrow_function_notation = args.use_arrow_function_notation;
  type_to_string.max_num_type_variables = args.max_num_type_variables;
  type_to_string.show_class_source_type = args.show_class_source_type;
  type_to_string.rich_text = args.rich_text;
  type_to_string.show_application_outputs = args.show_application_outputs;
}

void show_parse_errors(const ParseErrors& errors,
                       const TokenSourceMap& source_data,
                       const cmd::Arguments& arguments) {
  ShowParseErrors show_errs;
  show_errs.is_rich_text = arguments.rich_text;
  show_errs.show(errors, source_data);
}

void show_type_errors(const TypeErrors& errors,
                      const TokenSourceMap& source_data,
                      const TypeToString& type_to_string,
                      const cmd::Arguments& arguments) {
  ShowTypeErrors show(type_to_string);
  show.show(errors, source_data);
}

void show_function_types(const FunctionsByFile& functions_by_file,
                         const TypeToString& type_to_string,
                         const Store& store,
                         const StringRegistry& string_registry,
                         const Library& library) {
  for (const auto& file_it : functions_by_file.store) {
    const auto& def_handles = file_it.second;
    std::cout << type_to_string.color(style::underline)
              << file_it.first->file_path
              << type_to_string.color(style::dflt) << std::endl;

    int64_t def_index = 1;
    for (const auto& def_handle : def_handles) {
      const auto maybe_type = library.lookup_local_function(def_handle);
      if (!maybe_type) {
        continue;
      }

      const auto& type = maybe_type.value();
      const auto func_ident = store.get_name(def_handle);
      const auto func_name = string_registry.at(func_ident.full_name());

      std::cout << mt::spaces(2) << (def_index++) << "."
                << type_to_string.apply(type) << std::endl;
    }

    std::cout << std::endl;
  }
}

void show_asts(const AstStore& ast_store,
               const Store& def_store,
               const StringRegistry& string_registry,
               const cmd::Arguments& arguments) {
  const bool rich_text = arguments.rich_text;

  for (const auto& entry : ast_store.asts) {
    if (entry.second.root_block) {
      std::cout << maybe_style(style::underline, rich_text)
                << entry.first
                << maybe_style(style::dflt, rich_text) << ": " << std::endl;

      StringVisitor visitor(&string_registry, &def_store);
      visitor.colorize = rich_text;
      visitor.include_def_ptrs = false;
      visitor.parenthesize_exprs = false;

      std::cout << entry.second.root_block->accept(visitor) << std::endl << std::endl;
    }
  }
}

}
