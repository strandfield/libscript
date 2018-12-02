// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/context.h"
#include "script/private/context_p.h"

#include "script/module.h"
#include "script/script.h"

#include "script/private/scope_p.h"

namespace script
{

Context::Context(const std::shared_ptr<ContextImpl> & impl)
  : d(impl)
{
  
}


int Context::id() const
{
  return d->id;
}

bool Context::isNull() const
{
  return d == nullptr;
}

Engine * Context::engine() const
{
  return d->engine;
}

const std::string & Context::name() const
{
  return d->name;
}

void Context::setName(const std::string & name)
{
  d->name = name;
}


const std::map<std::string, Value> & Context::vars() const
{
  return d->variables;
}

void Context::addVar(const std::string & name, const Value & val)
{
  d->variables[name] = val;
}

bool Context::exists(const std::string & name) const
{
  return d->variables.find(name) != d->variables.end();
}

Value Context::get(const std::string & name) const
{
  auto it = d->variables.find(name);
  if (it == d->variables.end())
    return Value{};
  return it->second;
}

void Context::use(const Module &m)
{
  d->scope.merge(m.scope());
}

void Context::use(const Script &s)
{
  d->scope.merge(Scope{ s.rootNamespace() });
}

Scope Context::scope() const
{
  return Scope{ std::make_shared<ContextScope>(*this, d->scope.impl()) };
}

void Context::clear()
{
  d->variables.clear();
}

} // namespace script
