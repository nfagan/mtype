#include "type_properties.hpp"
#include "type_store.hpp"
#include "debug.hpp"
#include <cassert>

namespace mt {

/*
 * IsConcreteArgument
 */

bool IsConcreteArgument::is_concrete_argument(const Type* type) {
  using Tag = Type::Tag;

  switch (type->tag) {
    case Tag::destructured_tuple:
      return is_concrete_argument(MT_DT_REF(*type));
    case Tag::list:
      return is_concrete_argument(MT_LIST_REF(*type));
    case Tag::abstraction:
      return is_concrete_argument(MT_ABSTR_REF(*type));
    case Tag::scheme:
      return is_concrete_argument(MT_SCHEME_REF(*type));
    case Tag::class_type:
      return is_concrete_argument(MT_CLASS_REF(*type));
    case Tag::alias:
      return is_concrete_argument(MT_ALIAS_REF(*type));
    case Tag::union_type:
      return is_concrete_argument(MT_UNION_REF(*type));
    case Tag::tuple:
    case Tag::scalar:
    case Tag::record:
    case Tag::constant_value:
      return true;
    default:
      return false;
  }
}

bool IsConcreteArgument::are_concrete_arguments(const TypePtrs& args) {
  for (const auto& arg : args) {
    if (!is_concrete_argument(arg)) {
      return false;
    }
  }
  return true;
}

bool IsConcreteArgument::is_concrete_argument(const types::DestructuredTuple& tup) {
  return are_concrete_arguments(tup.members);
}

bool IsConcreteArgument::is_concrete_argument(const mt::types::Abstraction&) {
  return true;
}

bool IsConcreteArgument::is_concrete_argument(const types::List& list) {
  return are_concrete_arguments(list.pattern);
}

bool IsConcreteArgument::is_concrete_argument(const types::Union& union_type) {
  return are_concrete_arguments(union_type.members);
}

bool IsConcreteArgument::is_concrete_argument(const types::Scheme& scheme) {
  return is_concrete_argument(scheme.type);
}

bool IsConcreteArgument::is_concrete_argument(const types::Class& class_type) {
  return is_concrete_argument(class_type.source);
}

bool IsConcreteArgument::is_concrete_argument(const types::Alias& alias) {
  return is_concrete_argument(alias.source);
}

/*
 * IsFullyConcrete
 */

bool IsFullyConcreteInstance::maybe_push_scheme_variables(const Type* in_type) {
  if (in_type->is_scheme()) {
    const auto& scheme = MT_SCHEME_REF(*in_type);
    const auto& scheme_vars = scheme.parameters;
    enclosing_scheme_variables.emplace_back(scheme_vars.begin(), scheme_vars.end());
    return true;
  } else {
    return false;
  }
}

IsFullyConcrete::IsFullyConcrete(IsFullyConcreteInstance* instance) :
instance(instance) {
  //
}

bool IsFullyConcrete::type_ptrs(const TypePtrs& types) const {
  bool success = true;

  for (const auto& t : types) {
    if (!t->accept(*this)) {
      success = false;
    }
  }

  return success;
}

void IsFullyConcrete::two_types(const Type* a, const Type* b, bool* success) const {
  if (!a->accept(*this)) {
    *success = false;
  }
  if (!b->accept(*this)) {
    *success = false;
  }
}

bool IsFullyConcrete::scheme(const types::Scheme& scheme) const {
  auto& scheme_vars = instance->enclosing_scheme_variables;
  scheme_vars.emplace_back(scheme.parameters.begin(), scheme.parameters.end());
  MT_SCOPE_EXIT {
    scheme_vars.pop_back();
  };

  return scheme.type->accept(*this);
}

bool IsFullyConcrete::abstraction(const types::Abstraction& abstr) const {
  bool success = true;
  two_types(abstr.inputs, abstr.outputs, &success);
  return success;
}

bool IsFullyConcrete::application(const types::Application& app) const {
  bool success = true;

  if (!app.abstraction->accept(*this)) {
    success = false;
  }

  two_types(app.inputs, app.outputs, &success);

  return success;
}

bool IsFullyConcrete::constant_value(const types::ConstantValue&) const {
  return true;
}

namespace {
  bool is_scheme_bound_variable(const IsFullyConcreteInstance& instance,
                                const Type* var_like) {
    const auto& scheme_vars = instance.enclosing_scheme_variables;
    for (int64_t i = scheme_vars.size()-1; i >= 0; i--) {
      if (scheme_vars[i].count(var_like) > 0) {
        return true;
      }
    }
    return false;
  }

  bool handle_var_like(IsFullyConcreteInstance& instance,
                       const Type* source) {
    if (is_scheme_bound_variable(instance, source)) {
      return true;
    } else {
      instance.unbound_variables.insert(source);
      return false;
    }
  }
}

bool IsFullyConcrete::variable(const types::Variable& var) const {
  return handle_var_like(*instance, &var);
}

bool IsFullyConcrete::parameters(const types::Parameters& params) const {
  return handle_var_like(*instance, &params);
}

bool IsFullyConcrete::list(const types::List& list) const {
  return type_ptrs(list.pattern);
}

bool IsFullyConcrete::tuple(const types::Tuple& tup) const {
  return type_ptrs(tup.members);
}

bool IsFullyConcrete::destructured_tuple(const types::DestructuredTuple& tup) const {
  return type_ptrs(tup.members);
}

bool IsFullyConcrete::union_type(const types::Union& union_type) const {
  return type_ptrs(union_type.members);
}

bool IsFullyConcrete::alias(const types::Alias& alias) const {
  return alias.source->accept(*this);
}

bool IsFullyConcrete::cast(const types::Cast& alias) const {
  bool success = true;
  two_types(alias.from, alias.to, &success);
  return success;
}

bool IsFullyConcrete::record(const types::Record& record) const {
  bool success = true;

  for (const auto& field : record.fields) {
    //  @Note: make sure to visit all types.
    two_types(field.name, field.type, &success);
  }

  return success;
}

bool IsFullyConcrete::class_type(const types::Class& class_type) const {
  return class_type.source->accept(*this);
}

bool IsFullyConcrete::scalar(const types::Scalar&) const {
  return true;
}

bool IsFullyConcrete::subscript(const types::Subscript& sub) const {
  bool success = true;

  two_types(sub.principal_argument, sub.outputs, &success);

  for (const auto& s : sub.subscripts) {
    if (!type_ptrs(s.arguments)) {
      success = false;
    }
  }

  return success;
}

bool IsFullyConcrete::assignment(const types::Assignment& assign) const {
  bool success = true;
  two_types(assign.lhs, assign.rhs, &success);
  return success;
}

/*
 * IsRecursive
 */

IsRecursiveInstance::IsRecursiveInstance() : IsRecursiveInstance(nullptr) {
  //
}

IsRecursiveInstance::IsRecursiveInstance(const RemappedAliasSources* remapped_alias_sources) :
is_recursive(false), remapped_alias_sources(remapped_alias_sources) {
  //
}

bool IsRecursiveInstance::visited(const Type* type) const {
  return visited_types.count(type) > 0;
}

void IsRecursiveInstance::unmark(const Type* type) {
  visited_types.erase(type);
}

void IsRecursiveInstance::mark(const Type* type) {
  visited_types.insert(type);
}

Optional<const Type*> IsRecursiveInstance::maybe_remapped_alias_source(const Type* alias) const {
  if (remapped_alias_sources) {
    auto it = remapped_alias_sources->find(alias);
    return it == remapped_alias_sources->end() ?
      NullOpt{} : Optional<const Type*>(it->second);
  } else {
    return NullOpt{};
  }
}

IsRecursive::IsRecursive(IsRecursiveInstance* instance) : instance(instance) {
  //
}

#define MT_CHECK_VISITED_EARLY_RETURN(var) \
  if (instance->visited((var))) { \
    instance->is_recursive = true; \
    return; \
  } else { \
    instance->mark((var)); \
  }

void IsRecursive::scheme(const types::Scheme& scheme) {
  MT_CHECK_VISITED_EARLY_RETURN(&scheme)
  scheme.type->accept_const(*this);
  instance->unmark(&scheme);
}

void IsRecursive::abstraction(const types::Abstraction& abstr) {
  MT_CHECK_VISITED_EARLY_RETURN(&abstr)
  abstr.inputs->accept_const(*this);
  abstr.outputs->accept_const(*this);
  instance->unmark(&abstr);
}

void IsRecursive::application(const types::Application& app) {
  MT_CHECK_VISITED_EARLY_RETURN(&app)

  app.abstraction->accept_const(*this);
  app.inputs->accept_const(*this);
  app.outputs->accept_const(*this);

  instance->unmark(&app);
}

void IsRecursive::constant_value(const types::ConstantValue& cv) {
  MT_CHECK_VISITED_EARLY_RETURN(&cv)
  instance->unmark(&cv);
}

void IsRecursive::alias(const types::Alias& alias) {
  MT_CHECK_VISITED_EARLY_RETURN(&alias)

  auto maybe_remapped_alias_source =
    instance->maybe_remapped_alias_source(&alias);

  if (maybe_remapped_alias_source) {
    maybe_remapped_alias_source.value()->accept_const(*this);
  } else {
    alias.source->accept_const(*this);
  }

  instance->unmark(&alias);
}

void IsRecursive::cast(const types::Cast& cast) {
  MT_CHECK_VISITED_EARLY_RETURN(&cast)

  cast.from->accept_const(*this);
  cast.to->accept_const(*this);
  instance->unmark(&cast);
}

void IsRecursive::record(const types::Record& record) {
  MT_CHECK_VISITED_EARLY_RETURN(&record)

  for (const auto& field : record.fields) {
    field.name->accept_const(*this);
    field.type->accept_const(*this);
  }

  instance->unmark(&record);
}

void IsRecursive::class_type(const types::Class& class_type) {
  MT_CHECK_VISITED_EARLY_RETURN(&class_type);
  class_type.source->accept_const(*this);
  instance->unmark(&class_type);
}

void IsRecursive::subscript(const types::Subscript& sub) {
  MT_CHECK_VISITED_EARLY_RETURN(&sub)
  sub.principal_argument->accept_const(*this);
  sub.outputs->accept_const(*this);
  for (const auto& s : sub.subscripts) {
    visit(s.arguments);
  }
  instance->unmark(&sub);
}

void IsRecursive::assignment(const types::Assignment& assign) {
  MT_CHECK_VISITED_EARLY_RETURN(&assign)
  assign.lhs->accept_const(*this);
  assign.rhs->accept_const(*this);
  instance->unmark(&assign);
}

void IsRecursive::list(const types::List& list) {
  MT_CHECK_VISITED_EARLY_RETURN(&list)
  visit(list.pattern);
  instance->unmark(&list);
}

void IsRecursive::tuple(const types::Tuple& tup) {
  MT_CHECK_VISITED_EARLY_RETURN(&tup)
  visit(tup.members);
  instance->unmark(&tup);
}

void IsRecursive::destructured_tuple(const types::DestructuredTuple& tup) {
  MT_CHECK_VISITED_EARLY_RETURN(&tup)
  visit(tup.members);
  instance->unmark(&tup);
}

void IsRecursive::union_type(const types::Union& union_type) {
  MT_CHECK_VISITED_EARLY_RETURN(&union_type)
  visit(union_type.members);
  instance->unmark(&union_type);
}

void IsRecursive::visit(const TypePtrs& type_ptrs) {
  for (const auto& type : type_ptrs) {
    type->accept_const(*this);
  }
}

}