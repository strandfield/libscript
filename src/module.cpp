// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/module.h"
#include "script/private/module_p.h"

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

ModuleImpl::ModuleImpl(Engine *e, const std::string & str)
  : NamespaceImpl(str, e)
  , load(module_default_load)
  , cleanup(module_default_cleanup)
  , loaded(false)
{

}

ModuleImpl::ModuleImpl(Engine *e, const std::string & module_name, ModuleLoadFunction load_callback, ModuleCleanupFunction cleanup_callback)
  : NamespaceImpl(module_name, e)
  , load(load_callback)
  , cleanup(cleanup_callback)
  , loaded(false)
{

}


Module::Module(const std::shared_ptr<ModuleImpl> & impl)
  : d(impl) {}

Engine * Module::engine() const
{
  return d->engine;
}

const std::string & Module::name() const
{
  return d->name;
}

Module Module::newSubModule(const std::string & name)
{
  Module m{ std::make_shared<ModuleImpl>(engine(), name) };
  d->modules.push_back(m);
  return m;
}

Module Module::newSubModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
{
  Module m{ std::make_shared<ModuleImpl>(engine(), name, load, cleanup) };
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

Namespace Module::root() const
{
  return Namespace{ d };
}

Scope Module::scope() const
{
  Scope scp{ Namespace{ d } };

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

  d->functions.clear();
  d->classes.clear();
  d->enums.clear();
  d->namespaces.clear();
  d->operators.clear();
  d->templates.clear();
  d->variables.clear();
  d->typedefs.clear();
}


} // namespace script