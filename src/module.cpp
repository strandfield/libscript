// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/module.h"
#include "script/private/module_p.h"

#include "script/engine.h"
#include "script/scope.h"

#include "script/private/namespace_p.h"

namespace script
{

static void module_default_load(Module m)
{
  for (Module child : m.submodules())
    child.load();
}

static void module_default_cleanup(Module)
{

}

ModuleImpl::ModuleImpl(const std::string & str, const Namespace &n)
  : name(str)
  , ns(n)
  , load(module_default_load)
  , cleanup(module_default_cleanup)
  , loaded(false)
{

}

ModuleImpl::ModuleImpl(const std::string & module_name, const Namespace & nmspc, ModuleLoadFunction load_callback, ModuleCleanupFunction cleanup_callback)
  : name(module_name)
  , ns(nmspc)
  , load(load_callback)
  , cleanup(cleanup_callback)
  , loaded(false)
{

}


Module::Module(const std::shared_ptr<ModuleImpl> & impl)
  : d(impl) {}

Engine * Module::engine() const
{
  return d->ns.engine();
}

const std::string & Module::name() const
{
  return d->name;
}

Module Module::newSubModule(const std::string & name)
{
  Namespace ns{ std::make_shared<NamespaceImpl>("", engine()) };
  Module m{ std::make_shared<ModuleImpl>(name, ns) };
  d->modules.push_back(m);
  return m;
}

Module Module::newSubModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
{
  Namespace ns{ std::make_shared<NamespaceImpl>("", engine()) };
  Module m{ std::make_shared<ModuleImpl>(name, ns, load, cleanup) };
  d->modules.push_back(m);
  return m;
}

Module Module::getSubModule(const std::string & name) const
{
  for (const auto & child : d->modules)
  {
    if (child.name() == name)
      return child;
  }

  return Module{};
}

const std::vector<Module> & Module::submodules() const
{
  return d->modules;
}

bool Module::isLoaded() const
{
  return d->loaded;
}

void Module::load()
{
  if (d->loaded)
    return;

  d->loaded = true;
  d->load(*this);
}

const Namespace & Module::root() const
{
  return d->ns;
}

Scope Module::scope() const
{
  Scope scp{ d->ns };

  for (const auto & child : d->modules)
  {
    if (!child.isLoaded())
      continue;

    Scope submodule_scope = child.scope();
    scp.merge(submodule_scope);
  }

  return scp;
}

void Module::destroy()
{
  d->cleanup(*this);

  for (auto child : d->modules)
  {
    child.destroy();
  }

  d->modules.clear();

  d->ns = Namespace{}; /// TODO : is that correct ?
}


} // namespace script