%{

Dispatch impl.

Consider an unresolved external function `x` applied in `scope` with
arguments of concrete type T1 ... Tn

struct PathLookupResult {
  bool resolved;
  CodeFileDescriptor resolved_file;
}

class AstStore {
  void emplace(code_file_descriptor, root_block)
}

struct TypeLookupResult {
  bool resolved;
  bool validated;
  Type* type;
}

Case 1. Primary function lookup:
  a) If x names a fully qualified import in scope, `path-search` for the 
     full name of x. Error if unresolved.
  b) For each wildcard import, `path-search` for the imported name + x.
     Ok if unresolved, proceed to c)
  c) If there's a private folder adjacent to the filepath of the
     CodeFileDescriptor of scope, `path-search` for the full name of x in
     that folder. Ok if unresolved, proceed to Case 2.

Case 2. Method dispatch:
  a) For each argument type `Ti`, consider whether x is a method of that
     type. See dispatch.

Sub-routine path-search (name, file_descriptor)

class SearchPath {
  std::vector<FilePath> directories;
}

---

void Unifier::check_push_func(TypeRef source, TermRef term, const types::Abstraction& func) {

  if (!is_concrete_argument(func.inputs) || func.is_anonymous()) {
    return;
  }

  auto func_info = function_info_for_type(func.reference_handle);

  if (func_info.is_fully_qualified_import()) {
    path_lookup(func_info.name, func_info.file_descriptor);
  }

}

void path_lookup(func_name, file_descriptor) {
  lookup_res = path_search(func_name, file_descriptor);
  if (!lookup_res.resolved) {
    report_error("No such function.");
    return;
  }

  type_lookup_res = type_store.lookup_file(lookup_res.resolved_file);
  if (!type_lookup_res.resolved) {
    add_pending_type(func_type_ptr, lookup_res.resolved_file);
    return;
  }
}

%}