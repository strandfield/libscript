// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/context.h"
#include "context_p.h"

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


void Context::clear()
{
  d->variables.clear();
}

ContextImpl * Context::implementation() const
{
  return d.get();
}

std::weak_ptr<ContextImpl> Context::weakref() const
{
  return std::weak_ptr<ContextImpl>{d};
}

} // namespace script
