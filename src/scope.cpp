// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/scope.h"
#include "scope_p.h"

#include "script/engine.h"
#include "class_p.h"
#include "script/enumvalue.h"
#include "namespace_p.h"
#include "script/operator.h"
#include "script_p.h"

namespace script
{


static Scope find_class_decl_routine(const Class & cla, const Scope & scp)
{
  const std::vector<Class> & classes = scp.classes();
  if (std::find(classes.begin(), classes.end(), cla) != classes.end())
    return scp;

  for (const Class & c : classes)
  {
    Scope ret = find_class_decl_routine(cla, Scope{ c, scp });
    if (!ret.isNull())
      return ret;
  }

  for (const Namespace & n : scp.namespaces())
  {
    Scope ret = find_class_decl_routine(cla, Scope{ n, scp });
    if (!ret.isNull())
      return ret;
  }

  return Scope{};
}

static Scope find_enum_decl_routine(const Enum & entity, const Scope & scp)
{
  const std::vector<Enum> & enumerations = scp.enums();
  if (std::find(enumerations.begin(), enumerations.end(), entity) != enumerations.end())
    return scp;

  const auto & classes = scp.classes();
  for (const Class & c : classes)
  {
    Scope ret = find_enum_decl_routine(entity, Scope{ c, scp });
    if (!ret.isNull())
      return ret;
  }

  for (const Namespace & n : scp.namespaces())
  {
    Scope ret = find_enum_decl_routine(entity, Scope{ n, scp });
    if (!ret.isNull())
      return ret;
  }

  return Scope{};
}


ScopeImpl::ScopeImpl(std::shared_ptr<ScopeImpl> p)
  : parent(p)
{

}


NamespaceScope::NamespaceScope(const Namespace & ns, std::shared_ptr<ScopeImpl> p)
  : ScopeImpl(p)
  , mNamespace(ns)
{

}

Engine * NamespaceScope::engine() const
{
  return mNamespace.engine();
}

int NamespaceScope::kind() const
{
  return Scope::NamespaceScope;
}

const std::vector<Class> & NamespaceScope::classes() const
{
  return mNamespace.classes();
}

const std::vector<Enum> & NamespaceScope::enums() const
{
  return mNamespace.enums();
}

const std::vector<Function> & NamespaceScope::functions() const
{
  return mNamespace.functions();
}

const std::vector<LiteralOperator> & NamespaceScope::literal_operators() const
{
  return mNamespace.literalOperators();
}

const std::vector<Namespace> & NamespaceScope::namespaces() const
{
  return mNamespace.namespaces();
}

const std::vector<Operator> & NamespaceScope::operators() const
{
  return mNamespace.operators();
}

const std::vector<Template> & NamespaceScope::templates() const
{
  return mNamespace.templates();
}

void NamespaceScope::add_class(const Class & c)
{
  mNamespace.implementation()->classes.push_back(c);
}

void NamespaceScope::add_function(const Function & f)
{
  mNamespace.implementation()->functions.push_back(f);
}

void NamespaceScope::add_operator(const Operator & op)
{
  mNamespace.implementation()->operators.push_back(op);
}

void NamespaceScope::add_literal_operator(const LiteralOperator & lo)
{
  mNamespace.implementation()->literal_operators.push_back(lo);
}

void NamespaceScope::add_enum(const Enum & e)
{
  mNamespace.implementation()->enums.push_back(e);
}


ClassScope::ClassScope(const Class & c, std::shared_ptr<ScopeImpl> p)
  : ScopeImpl(p)
  , mClass(c)
{

}

Engine * ClassScope::engine() const
{
  return mClass.engine();
}

int ClassScope::kind() const
{
  return Scope::ClassScope;
}

const std::vector<Class> & ClassScope::classes() const
{
  return mClass.classes();
}

const std::vector<Enum> & ClassScope::enums() const
{
  return mClass.enums();
}

const std::vector<Function> & ClassScope::functions() const
{
  return mClass.memberFunctions();
}

const std::vector<LiteralOperator> ClassScope::static_dummy_literal_operators = std::vector<LiteralOperator>{};

const std::vector<LiteralOperator> & ClassScope::literal_operators() const
{
  return static_dummy_literal_operators;
}

const std::vector<Namespace> & ClassScope::namespaces() const
{
  return static_dummy_namespaces;
}

const std::vector<Namespace> ClassScope::static_dummy_namespaces = std::vector<Namespace>{};

const std::vector<Operator> & ClassScope::operators() const
{
  return mClass.operators();
}

const std::vector<Template> & ClassScope::templates() const
{
  return mClass.templates();
}

void ClassScope::add_class(const Class & c)
{
  mClass.implementation()->classes.push_back(c);
}

void ClassScope::add_function(const Function & f)
{
  mClass.implementation()->register_function(f);
}

void ClassScope::add_operator(const Operator & op)
{
  mClass.implementation()->operators.push_back(op);
}

void ClassScope::add_cast(const Cast & c)
{
  mClass.implementation()->casts.push_back(c);
}

void ClassScope::add_enum(const Enum & e)
{
  mClass.implementation()->enums.push_back(e);
}


EnumScope::EnumScope(const Enum & e, std::shared_ptr<ScopeImpl> p)
  : ScopeImpl(p)
  , mEnum(e)
{

}

Engine * EnumScope::engine() const
{
  return mEnum.engine();
}

int EnumScope::kind() const
{
  return Scope::EnumClassScope;
}

const std::vector<Class> EnumScope::static_dummy_classes = std::vector<Class>{};

const std::vector<Class> & EnumScope::classes() const
{
  return static_dummy_classes;
}

const std::vector<Enum> EnumScope::static_dummy_enums = std::vector<Enum>{};

const std::vector<Enum> & EnumScope::enums() const
{
  return static_dummy_enums;
}

const std::vector<Function> EnumScope::static_dummy_functions = std::vector<Function>{};

const std::vector<Function> & EnumScope::functions() const
{
  return static_dummy_functions;
}

const std::vector<LiteralOperator> EnumScope::static_dummy_literal_operators = std::vector<LiteralOperator>{};

const std::vector<LiteralOperator> & EnumScope::literal_operators() const
{
  return static_dummy_literal_operators;
}

const std::vector<Namespace> EnumScope::static_dummy_namespaces = std::vector<Namespace>{};

const std::vector<Namespace> & EnumScope::namespaces() const
{
  return static_dummy_namespaces;
}

const std::vector<Operator> EnumScope::static_dummy_operators = std::vector<Operator>{};

const std::vector<Operator> & EnumScope::operators() const
{
  return static_dummy_operators;
}

const std::vector<Template> EnumScope::static_dummy_templates = std::vector<Template>{};

const std::vector<Template> & EnumScope::templates() const
{
  return static_dummy_templates;
}

ScriptScope::ScriptScope(const Script & s, std::shared_ptr<ScopeImpl> p)
  : ScopeImpl(p)
  , mScript(s)
{

}

Engine * ScriptScope::engine() const
{
  return mScript.engine();
}

int ScriptScope::kind() const
{
  return Scope::ScriptScope;
}

const std::vector<Class> & ScriptScope::classes() const
{
  return mScript.classes();
}

const std::vector<Enum> & ScriptScope::enums() const
{
  return mScript.rootNamespace().enums();
}

const std::vector<Function> & ScriptScope::functions() const
{
  return mScript.rootNamespace().functions();
}

const std::vector<LiteralOperator> & ScriptScope::literal_operators() const
{
  return mScript.rootNamespace().literalOperators();
}


const std::vector<Namespace> & ScriptScope::namespaces() const
{
  return mScript.rootNamespace().namespaces();
}

const std::vector<Operator> & ScriptScope::operators() const
{
  return mScript.rootNamespace().operators();
}

const std::vector<Template> & ScriptScope::templates() const
{
  return mScript.rootNamespace().templates();
}

void ScriptScope::add_class(const Class & c)
{
  mScript.rootNamespace().implementation()->classes.push_back(c);
}

void ScriptScope::add_function(const Function & f)
{
  mScript.rootNamespace().implementation()->functions.push_back(f);
}

void ScriptScope::add_operator(const Operator & op)
{
  mScript.rootNamespace().implementation()->operators.push_back(op);
}

void ScriptScope::add_literal_operator(const LiteralOperator & lo)
{
  mScript.rootNamespace().implementation()->literal_operators.push_back(lo);
}

void ScriptScope::add_enum(const Enum & e)
{
  mScript.rootNamespace().implementation()->enums.push_back(e);
}

void ScriptScope::remove_class(const Class & c)
{
  auto & container = mScript.rootNamespace().implementation()->classes;
  auto it = std::find(container.begin(), container.end(), c);
  container.erase(it);
}

void ScriptScope::remove_enum(const Enum & e)
{
  auto & container = mScript.rootNamespace().implementation()->enums;
  auto it = std::find(container.begin(), container.end(), e);
  container.erase(it);
}



Scope::Scope(const Enum & e, const Scope & parent)
{
  d = std::make_shared<script::EnumScope>(e, parent.impl());
}

Scope::Scope(const Class & cla, const Scope & parent)
{
  d = std::make_shared<script::ClassScope>(cla, parent.impl());
}

Scope::Scope(const Namespace & na, const Scope & parent)
{
  d = std::make_shared<script::NamespaceScope>(na, parent.impl());
}

Scope::Scope(const Script & s, const Scope & parent)
{
  d = std::make_shared<script::ScriptScope>(s, parent.impl());
}

Scope::Scope(const std::shared_ptr<ScopeImpl> & impl)
  : d(impl)
{

}

bool Scope::isNull() const
{
  return d == nullptr;
}

Scope::Type Scope::type() const
{
  if (isNull())
    return InvalidScope;
  return static_cast<Scope::Type>(d->kind());
}


Engine * Scope::engine() const
{
  return d->engine();
}

bool Scope::hasParent() const
{
  return d->parent != nullptr;
}

Scope Scope::parent() const
{
  return Scope{ d->parent };
}


bool Scope::isClass() const
{
  return dynamic_cast<const script::ClassScope*>(d.get()) != nullptr;
}

Class Scope::asClass() const
{
  return std::dynamic_pointer_cast<script::ClassScope>(d)->mClass;
}

Namespace Scope::asNamespace() const
{
  return std::dynamic_pointer_cast<script::NamespaceScope>(d)->mNamespace;
}

Enum Scope::asEnum() const
{
  return std::dynamic_pointer_cast<script::EnumScope>(d)->mEnum;
}

Script Scope::asScript() const
{
  return std::dynamic_pointer_cast<script::ScriptScope>(d)->mScript;
}


const std::vector<Class> & Scope::classes() const
{
  return d->classes();
}

const std::vector<Enum> & Scope::enums() const
{
  return d->enums();
}

const std::vector<Function> & Scope::functions() const
{
  return d->functions();
}

const std::vector<Namespace> & Scope::namespaces() const
{
  return d->namespaces();
}

const std::vector<Operator> & Scope::operators() const
{
  return d->operators();
}

const std::vector<LiteralOperator> & Scope::literalOperators() const
{
  return d->literal_operators();
}

const std::vector<Template> & Scope::templates() const
{
  return d->templates();
}

std::vector<Function> Scope::operators(Operator::BuiltInOperator op) const
{
  const std::vector<Operator> & candidates = operators();
  std::vector<Function> ret;
  for (size_t i(0); i < candidates.size(); ++i)
  {
    if (candidates.at(i).operatorId() != op)
      continue;
    ret.push_back(candidates.at(i));
  }

  return ret;
}

Scope Scope::child(const std::string & name) const
{
  if (isNull())
    return Scope{};

  for (const auto & cla : this->classes())
  {
    if (cla.name() == name)
      return Scope{ cla, *this };
  }

  for (const auto & n : this->namespaces())
  {
    if (n.name() == name)
      return Scope{ n, *this };
  }

  for (const auto & e : this->enums())
  {
    if (e.name() == name)
      return Scope{ e, *this };
  }

  return Scope{};
}

Scope Scope::find(const Class & c)
{
  if (c.isNull())
    throw std::runtime_error{ "Invalid class" };

  Engine *e = c.engine();

  Scope scp = find_class_decl_routine(c, Scope{ e->rootNamespace() });
  if (!scp.isNull())
    return scp;

  for (const Script & s : e->scripts())
  {
    scp = find_class_decl_routine(c, Scope{ s });
    if (!scp.isNull())
      return scp;
  }

  return Scope{};
}

Scope Scope::find(const Enum & entity)
{
  if (entity.isNull())
    throw std::runtime_error{ "Invalid enum" };

  Engine *e = entity.engine();

  Scope scp = find_enum_decl_routine(entity, Scope{ e->rootNamespace() });
  if (!scp.isNull())
    return scp;

  for (const Script & s : e->scripts())
  {
    scp = find_enum_decl_routine(entity, Scope{ s });
    if (!scp.isNull())
      return scp;
  }

  return Scope{};
}

const std::shared_ptr<ScopeImpl> & Scope::impl() const
{
  return d;
}

} // namespace script