#pragma once

#include "types.hpp"

namespace mt {

class Unifier;
class Library;

class SubscriptHandler {
public:
  explicit SubscriptHandler(Unifier& unifier);
  void maybe_unify_subscript(Type* source, TermRef term, types::Subscript& sub);

private:
  void principal_argument_dispatch(Type* source,
                                   TermRef term,
                                   types::Subscript& sub, Type* arg0);

  void maybe_unify_non_function_subscript(Type* source, TermRef term,
                                          types::Subscript& sub,
                                          Type* arg0);

  Type* record_period_subscript(Type* record_source,
                                const Token* source_token,
                                const types::Subscript::Sub& sub0,
                                const types::Record& arg0);

  Type* match_members(const Type* source, const Token* source_token,
                      const TypePtrs& members);
  Type* tuple_brace_subscript(const Type* source, const Token* source_token,
                              const types::Tuple& arg0);
  Type* list_parens_subscript(const Type* source, const Token* source_token,
                              const types::List& arg0);

  void scheme_function_call_subscript(Type* source, TermRef term,
                                      const types::Scheme& scheme, types::Subscript& sub);
  void function_call_subscript(Type* source, TermRef term,
                               const types::Subscript& sub, Type* arg0);

  bool arguments_have_subsindex_defined(const TypePtrs& arguments) const;
  bool are_valid_subscript_arguments(const Type* arg0, const types::Subscript::Sub& sub) const;
  bool has_custom_subsref_method(const Type* arg0) const;
private:
  Unifier& unifier;
  const Library& library;
};

}