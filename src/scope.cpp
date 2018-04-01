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

static Scope find_namespace_decl_routine(const Namespace & entity, const Scope & scp)
{
  const std::vector<Namespace> & namespaces = scp.namespaces();
  if (std::find(namespaces.begin(), namespaces.end(), entity) != namespaces.end())
    return scp;

  for (const Namespace & n : scp.namespaces())
  {
    Scope ret = find_namespace_decl_routine(entity, Scope{ n, scp });
    if (!ret.isNull())
      return ret;
  }

  return Scope{};
}


ScopeImpl::ScopeImpl(Enum e, Scope p)
  : parent(p.impl())
  , theEnum(e)
{

}

ScopeImpl::ScopeImpl(Class c, Scope p)
  : parent(p.impl())
  , theClass(c)
{

}

ScopeImpl::ScopeImpl(Namespace n, Scope p)
  : parent(p.impl())
  , theNamespace(n)
{

}

ScopeImpl::ScopeImpl(Script s, Scope p)
  : parent(p.impl())
  , theScript(s)
{

}

void ScopeImpl::addClass(const Class & c)
{
  if (!theClass.isNull())
    theClass.implementation()->classes.push_back(c);
  else if (!theNamespace.isNull())
    theNamespace.implementation()->classes.push_back(c);
  else if (!theScript.isNull())
    theScript.rootNamespace().implementation()->classes.push_back(c);
}

void ScopeImpl::addFunction(const Function & f)
{
  if (!theClass.isNull())
    theClass.implementation()->register_function(f);
  else if (!theNamespace.isNull())
    theNamespace.implementation()->functions.push_back(f);
  else if (!theScript.isNull())
    theScript.rootNamespace().implementation()->functions.push_back(f);
}

void ScopeImpl::addOperator(const Operator & op)
{
  if (!theClass.isNull())
    theClass.implementation()->operators.push_back(op);
  else if (!theNamespace.isNull())
    theNamespace.implementation()->operators.push_back(op);
  else if (!theScript.isNull())
    theScript.rootNamespace().implementation()->operators.push_back(op);
}

void ScopeImpl::add_literal_operator(const LiteralOperator & lo)
{
  if (!theClass.isNull())
    throw std::runtime_error{ "Implementation error : ScopeImpl::add_literal_operator()" };
  else if (!theNamespace.isNull())
    theNamespace.implementation()->literal_operators.push_back(lo);
  else if (!theScript.isNull())
    theScript.rootNamespace().implementation()->literal_operators.push_back(lo);
}

void ScopeImpl::addCast(const Cast & c)
{
  assert(!theClass.isNull());
  theClass.implementation()->casts.push_back(c);
}

void ScopeImpl::addEnum(const Enum & e)
{
  if (!theClass.isNull())
    theClass.implementation()->enums.push_back(e);
  else if (!theNamespace.isNull())
    theNamespace.implementation()->enums.push_back(e);
  else if (!theScript.isNull())
    theScript.rootNamespace().implementation()->enums.push_back(e);
}

void ScopeImpl::removeClass(const Class & c)
{
  if (!theClass.isNull())
  {
    auto & container = theClass.implementation()->classes;
    auto it = std::find(container.begin(), container.end(), c);
    container.erase(it);
  }
  else if (!theNamespace.isNull())
  {
    auto & container = theNamespace.implementation()->classes;
    auto it = std::find(container.begin(), container.end(), c);
    container.erase(it);
  }
  else if (!theScript.isNull())
  {
    auto & container = theScript.rootNamespace().implementation()->classes;
    auto it = std::find(container.begin(), container.end(), c);
    container.erase(it);
  }
}

void ScopeImpl::removeFunction(const Function & f)
{
  if (f.isOperator())
    return removeOperator(f.toOperator());
  else if (f.isCast())
    return removeCast(f.toCast());

  if (!theClass.isNull())
  {
    if (f.isConstructor())
    {
      auto & container = theClass.implementation()->constructors;
      auto it = std::find(container.begin(), container.end(), f);
      container.erase(it);
    }
    else if (f.isDestructor())
    {
      theClass.implementation()->destructor = Function{};
    }
    else
    {
      auto & container = theClass.implementation()->functions;
      auto it = std::find(container.begin(), container.end(), f);
      container.erase(it);
    }
  }
  else if (!theNamespace.isNull())
  {
    auto & container = theNamespace.implementation()->functions;
    auto it = std::find(container.begin(), container.end(), f);
    container.erase(it);
  }
  else if (!theScript.isNull())
  {
    auto & container = theScript.rootNamespace().implementation()->functions;
    auto it = std::find(container.begin(), container.end(), f);
    container.erase(it);
  }
}

void ScopeImpl::removeOperator(const Operator & op)
{
  if (!theClass.isNull())
  {
    auto & container = theClass.implementation()->operators;
    auto it = std::find(container.begin(), container.end(), op);
    container.erase(it);
  }
  else if (!theNamespace.isNull())
  {
    auto & container = theNamespace.implementation()->operators;
    auto it = std::find(container.begin(), container.end(), op);
    container.erase(it);
  }
  else if (!theScript.isNull())
  {
    auto & container = theScript.rootNamespace().implementation()->operators;
    auto it = std::find(container.begin(), container.end(), op);
    container.erase(it);
  }
}

void ScopeImpl::removeCast(const Cast & c)
{
  if (!theClass.isNull())
  {
    auto & container = theClass.implementation()->casts;
    auto it = std::find(container.begin(), container.end(), c);
    container.erase(it);
  }
}

void ScopeImpl::removeEnum(const Enum & e)
{
  if (!theClass.isNull())
  {
    auto & container = theClass.implementation()->enums;
    auto it = std::find(container.begin(), container.end(), e);
    container.erase(it);
  }
  else if (!theNamespace.isNull())
  {
    auto & container = theNamespace.implementation()->enums;
    auto it = std::find(container.begin(), container.end(), e);
    container.erase(it);
  }
  else if (!theScript.isNull())
  {
    auto & container = theScript.rootNamespace().implementation()->enums;
    auto it = std::find(container.begin(), container.end(), e);
    container.erase(it);
  }
}




Scope::Scope(const Enum & e, const Scope & parent)
{
  d = std::make_shared<ScopeImpl>(e, parent);
}

Scope::Scope(const Class & cla, const Scope & parent)
{
  d = std::make_shared<ScopeImpl>(cla, parent);
}

Scope::Scope(const Namespace & na, const Scope & parent)
{
  d = std::make_shared<ScopeImpl>(na, parent);
}

Scope::Scope(const Script & s, const Scope & parent)
{
  d = std::make_shared<ScopeImpl>(s, parent);
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
  else if (!d->theClass.isNull())
    return ClassScope;
  else if (!d->theNamespace.isNull())
    return NamespaceScope;
  else if (!d->theScript.isNull())
    return ScriptScope;

  return InvalidScope;
}


Engine * Scope::engine() const
{
  if (!d->theClass.isNull())
    return d->theClass.engine();
  else if (!d->theNamespace.isNull())
    return d->theNamespace.engine();
  else if (!d->theScript.isNull())
    return d->theScript.engine();

  throw std::runtime_error{ "Call of engine() on a null FunctionScope" };
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
  return !d->theClass.isNull();
}

Class Scope::asClass() const
{
  return d->theClass;
}

Namespace Scope::asNamespace() const
{
  return d->theNamespace;
}

Enum Scope::asEnum() const
{
  return d->theEnum;
}

Script Scope::asScript() const
{
  return d->theScript;
}


std::vector<Class> Scope::classes() const
{
  if (!d->theClass.isNull())
    return d->theClass.classes();
  else if (!d->theNamespace.isNull())
    return d->theNamespace.classes();
  else if (!d->theScript.isNull())
    return d->theScript.classes();
  return {};
}

std::vector<Enum> Scope::enums() const
{
  if (!d->theClass.isNull())
    return d->theClass.enums();
  else if (!d->theNamespace.isNull())
    return d->theNamespace.enums();
  else if (!d->theScript.isNull())
    return d->theScript.rootNamespace().enums();
  return {};
}

std::vector<Function> Scope::functions() const
{
  if (!d->theNamespace.isNull())
    return d->theNamespace.functions();
  else if (!d->theClass.isNull())
    return d->theClass.memberFunctions();
  return {};
}

std::vector<Namespace> Scope::namespaces() const
{
  if (!d->theNamespace.isNull())
    return d->theNamespace.namespaces();
  else if (!d->theScript.isNull())
    return d->theScript.rootNamespace().namespaces();
  return {};
}

std::vector<Operator> Scope::operators() const
{
  if (!d->theNamespace.isNull())
    return d->theNamespace.operators();
  else if (!d->theClass.isNull())
    return d->theClass.operators();
  else if (!d->theScript.isNull())
    return d->theScript.operators();
  return {};
}

std::vector<LiteralOperator> Scope::literalOperators() const
{
  if (!d->theNamespace.isNull())
    return d->theNamespace.literalOperators();
  else if (!d->theScript.isNull())
    return d->theScript.rootNamespace().literalOperators();
  return {};
}

std::vector<Operator> Scope::operators(Operator::BuiltInOperator op) const
{
  std::vector<Operator> candidates = operators();
  for (size_t i(0); i < candidates.size(); ++i)
  {
    if (candidates.at(i).operatorId() != op)
    {
      std::swap(candidates[i], candidates.back());
      candidates.pop_back();
      --i;
    }
  }

  return candidates;
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

Scope Scope::find(const Namespace & entity)
{
  if (entity.isNull())
    throw std::runtime_error{ "Invalid namespace" };

  Engine *e = entity.engine();

  Scope scp = find_namespace_decl_routine(entity, Scope{ e->rootNamespace() });
  if (!scp.isNull())
    return scp;

  for (const Script & s : e->scripts())
  {
    scp = find_namespace_decl_routine(entity, Scope{ s });
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