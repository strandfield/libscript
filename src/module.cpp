// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/module.h"
#include "script/private/module_p.h"

#include "script/engine.h"
#include "script/scope.h"
#include "script/script.h"

#include "script/compiler/compiler.h"

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

NativeModule::NativeModule(Engine *e, const std::string & str)
  : NamespaceImpl(str, e)
  , load(module_default_load)
  , cleanup(module_default_cleanup)
  , loaded(false)
{

}

NativeModule::NativeModule(Engine *e, const std::string & module_name, ModuleLoadFunction load_callback, ModuleCleanupFunction cleanup_callback)
  : NamespaceImpl(module_name, e)
  , load(load_callback)
  , cleanup(cleanup_callback)
  , loaded(false)
{

}

bool NativeModule::is_module() const
{
  return true;
}

bool NativeModule::is_native_module() const
{
  return true;
}

ScriptModule::ScriptModule(int id, Engine *e, const SourceFile & src, const std::string & module_name)
  : ScriptImpl(id, e, src)
{
  this->name = module_name;
}

bool ScriptModule::is_module() const
{
  return true;
}


Module::Module(const std::shared_ptr<NamespaceImpl> & impl)
  : d(impl) 
{

}

Engine * Module::engine() const
{
  return d->engine;
}

const std::string & Module::name() const
{
  return d->name;
}

bool Module::isNative() const
{
  return d->is_native_module();
}

Module Module::newSubModule(const std::string & name)
{
  if (!isNative())
    throw std::runtime_error{ "Only native modules can have submodules" };

  NativeModule *nm = static_cast<NativeModule*>(impl());

  Module m{ std::make_shared<NativeModule>(engine(), name) };
  nm->modules.push_back(m);
  return m;
}

Module Module::newSubModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
{
  if (!isNative())
    throw std::runtime_error{ "Only native modules can have submodules" };

  NativeModule *nm = static_cast<NativeModule*>(impl());

  Module m{ std::make_shared<NativeModule>(engine(), name, load, cleanup) };
  nm->modules.push_back(m);
  return m;
}

Module Module::getSubModule(const std::string & name) const
{
  if (!isNative())
    return Module{};

  const NativeModule *nm = static_cast<const NativeModule*>(impl());

  for (const auto & child : nm->modules)
  {
    if (child.name() == name)
      return child;
  }

  return Module{};
}

const std::vector<Module> & Module::submodules() const
{
  static std::vector<Module> static_default = {};

  if (!isNative())
    return static_default;

  return static_cast<const NativeModule*>(impl())->modules;
}

bool Module::isLoaded() const
{
  if(isNative())
    return static_cast<const NativeModule*>(impl())->loaded;
  else
    return static_cast<const ScriptModule*>(impl())->loaded;
}

void Module::load()
{
  if (isLoaded())
    return;

  if (isNative())
  {
    auto nm = static_cast<NativeModule*>(impl());
    nm->loaded = true;
    nm->load(*this);
  }
  else
  {
    Script s = asScript();
    const bool success = s.compile();
    if (success && !engine()->compiler()->hasActiveSession())
      s.run();
  }
}

Namespace Module::root() const
{
  return Namespace{ d };
}

Scope Module::scope() const
{
  Scope scp{ Namespace{ d } };

  if (isNative())
  {
    for (const auto & child : submodules())
    {
      if (!child.isLoaded())
        continue;

      Scope submodule_scope = child.scope();
      scp.merge(submodule_scope);
    }
  }
  else
  {
    Script s = asScript();
    if (!s.exports().isNull())
      scp.merge(s.exports());
  }

  return scp;
}

Script Module::asScript() const
{
  if (isNative())
    return Script{};

  return Script{ std::static_pointer_cast<ScriptImpl>(d) };
}

void Module::destroy()
{
  if (isNative())
  {
    auto nm = static_cast<NativeModule*>(impl());

    nm->cleanup(*this);

    for (auto child : nm->modules)
    {
      child.destroy();
    }

    nm->modules.clear();

    nm->functions.clear();
    nm->classes.clear();
    nm->enums.clear();
    nm->namespaces.clear();
    nm->operators.clear();
    nm->templates.clear();
    nm->variables.clear();
    nm->typedefs.clear();
  }
}


} // namespace script