// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/module.h"
#include "script/private/module_p.h"

#include "script/engine.h"
#include "script/scope.h"
#include "script/script.h"

#include "script/compiler/compiler.h"

#include "script/private/engine_p.h"
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


ModuleLoadingError::ModuleLoadingError(std::string mssg)
  : message(std::move(mssg))
{

}

const char* ModuleLoadingError::what() const noexcept
{
  return "module-loading-error";
}

/*!
 * \class Module
 * \brief Provides module features
 */

Module::Module(const std::shared_ptr<NamespaceImpl> & impl)
  : d(impl) 
{

}

/*!
 * \fn Engine* engine() const
 * \brief Returns the engine that was used to construct this module
 */
Engine* Module::engine() const
{
  return d->engine;
}

/*!
 * \fn const std::string& name() const
 * \brief Returns the module name
 */
const std::string & Module::name() const
{
  return d->name;
}

/*!
 * \fn bool isNative() const
 * \brief Returns whether this module is a "native" module.
 * 
 * Unlike a "script" module, a native module has a C++ backend.
 */
bool Module::isNative() const
{
  return d->is_native_module();
}

/*!
 * \fn Module newSubModule(const std::string& name)
 * \brief Adds a submodule to this module.
 * 
 * This function creates a native submodule that defines no symbol but 
 * in which submodule can be added.
 * Use this function if you want to group modules in a common parent module.
 */
Module Module::newSubModule(const std::string & name)
{
  if (!isNative())
    throw std::runtime_error{ "Only native modules can have submodules" };

  NativeModule *nm = static_cast<NativeModule*>(impl());

  Module m{ std::make_shared<NativeModule>(engine(), name) };
  nm->modules.push_back(m);
  return m;
}

/*!
 * \fn Module newSubModule(const std::string& name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
 * \brief Adds a submodule to this module.
 *
 * This function creates a native submodule. The callbacks \a load and \a cleanup are 
 * called when the module is loaded and destroyed.
 */
Module Module::newSubModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
{
  if (!isNative())
    throw std::runtime_error{ "Only native modules can have submodules" };

  NativeModule *nm = static_cast<NativeModule*>(impl());

  Module m{ std::make_shared<NativeModule>(engine(), name, load, cleanup) };
  nm->modules.push_back(m);
  return m;
}

/*!
 * \fn Module newSubModule(const std::string& name, const SourceFile& src)
 * \brief Adds a script module to this module.
 *
 * This function creates a script submodule.
 * When the module is loaded, \a src is compiled.
 */
Module Module::newSubModule(const std::string& name, const SourceFile& src)
{
  if (!isNative())
    throw std::runtime_error{ "Only native modules can have submodules" };

  NativeModule* nm = static_cast<NativeModule*>(impl());

  auto mimpl = std::make_shared<ScriptModule>(static_cast<int>(engine()->implementation()->scripts.size()), engine(), src, name);
  engine()->implementation()->scripts.push_back(Script(mimpl));

  Module m{ mimpl };
  nm->modules.push_back(m);
  return m;
}

/*!
 * \fn Module getSubModule(const std::string& name) const
 * \brief Retrieves an existing submodule by name.
 */
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

/*!
 * \fn const std::vector<Module>& submodules() const
 * \brief Returns the module' submodules.
 */
const std::vector<Module> & Module::submodules() const
{
  static std::vector<Module> static_default = {};

  if (!isNative())
    return static_default;

  return static_cast<const NativeModule*>(impl())->modules;
}

/*!
 * \fn bool isLoaded() const
 * \brief Returns whether the module is loaded.
 */
bool Module::isLoaded() const
{
  if(isNative())
    return static_cast<const NativeModule*>(impl())->loaded;
  else
    return static_cast<const ScriptModule*>(impl())->loaded;
}

/*!
 * \fn void load()
 * \brief Loads the module.
 * Throws a ModuleLoadingError on failure.
 * Warning: calling this function while a module is loading is undefined behavior
 */
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

    if(!success)
      throw ModuleLoadingError{ "script compilation failed" };

    try
    {
      s.run();
    }
    catch (const RuntimeError&)
    {
      // @TODO: reproduce the error message
      throw ModuleLoadingError{ "script execution failed" };
    }
  }
}

/*!
 * \fn Namespace root() const
 * \brief Returns the module's root namespace
 */
Namespace Module::root() const
{
  return Namespace{ d };
}

/*!
 * \fn Scope scope() const
 * \brief Returns the module's scope.
 *
 * This scope contains all the symbols that are visible when the module 
 * is loaded. 
 * This includes submodules (native modules) and symbols exported with 
 * and "export import" statement (script modules).
 */
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

/*!
 * \fn Script asScript() const
 * \brief Returns the module's Script.
 *
 * If the module is a native module, this returns an invalid script.
 */
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

    if(isLoaded())
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