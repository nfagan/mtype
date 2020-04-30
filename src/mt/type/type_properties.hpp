#pragma once

#include "types.hpp"
#include "type_visitor.hpp"
#include <unordered_set>

namespace mt {

/*
 * IsConcreteArgument
 */

class IsConcreteArgument {
public:
  IsConcreteArgument() = delete;

  static bool is_concrete_argument(const Type* arg);
  static bool are_concrete_arguments(const TypePtrs& args);

private:
  static bool is_concrete_argument(const types::Scheme& scheme);
  static bool is_concrete_argument(const types::Abstraction& abstr);
  static bool is_concrete_argument(const types::DestructuredTuple& tup);
  static bool is_concrete_argument(const types::List& list);
  static bool is_concrete_argument(const types::Union& union_type);
  static bool is_concrete_argument(const types::Class& class_type);
  static bool is_concrete_argument(const types::Alias& alias);
};

/*
 * IsFullyConcrete
 */

struct IsFullyConcreteInstance {
  bool maybe_push_scheme_variables(const Type* in_type);

  std::unordered_set<const Type*> unbound_variables;
  std::vector<std::unordered_set<const Type*>> enclosing_scheme_variables;
};

class IsFullyConcrete {
public:
  IsFullyConcrete(IsFullyConcreteInstance* instance);

  bool scheme(const types::Scheme& scheme) const;
  bool abstraction(const types::Abstraction& abstr) const;
  bool application(const types::Application& app) const;
  bool constant_value(const types::ConstantValue& cv) const;
  bool variable(const types::Variable& var) const;
  bool parameters(const types::Parameters& params) const;
  bool list(const types::List& list) const;
  bool tuple(const types::Tuple& tup) const;
  bool destructured_tuple(const types::DestructuredTuple& tup) const;
  bool union_type(const types::Union& union_type) const;
  bool alias(const types::Alias& alias) const;
  bool record(const types::Record& record) const;
  bool class_type(const types::Class& class_type) const;
  bool scalar(const types::Scalar& scalar) const;
  bool subscript(const types::Subscript& sub) const;
  bool assignment(const types::Assignment& assign) const;

private:
  bool type_ptrs(const TypePtrs& types) const;
  void two_types(const Type* a, const Type* b, bool* success) const;

private:
  IsFullyConcreteInstance* instance;
};

/*
 * IsRecursive
 */

struct IsRecursiveInstance {
  using RemappedAliasSources = std::unordered_map<const Type*, const Type*>;

  IsRecursiveInstance();
  IsRecursiveInstance(const RemappedAliasSources* remapped_alias_sources);

  void mark(const Type* type);
  bool visited(const Type* type) const;
  Optional<const Type*> maybe_remapped_alias_source(const Type* alias) const;

  bool is_recursive;
  std::unordered_set<const Type*> visited_types;
  const RemappedAliasSources* remapped_alias_sources;
};

class IsRecursive : public TypeVisitor {
public:
  IsRecursive(IsRecursiveInstance* instance);

  void scheme(const types::Scheme& scheme) override;
  void abstraction(const types::Abstraction& abstr) override;
  void application(const types::Application& app) override;
  void constant_value(const types::ConstantValue& cv) override;
  void list(const types::List& list) override;
  void tuple(const types::Tuple& tup) override;
  void destructured_tuple(const types::DestructuredTuple& tup) override;
  void union_type(const types::Union& union_type) override;
  void alias(const types::Alias& alias) override;
  void record(const types::Record& record) override;
  void class_type(const types::Class& class_type) override;
  void subscript(const types::Subscript& sub) override;
  void assignment(const types::Assignment& assign) override;

private:
  void visit(const TypePtrs& type_ptrs);

private:
  IsRecursiveInstance* instance;
};

}