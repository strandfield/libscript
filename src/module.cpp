// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/module.h"

#include "script/module-interface.h"
#include "script/group-module.h"
#include "script/script-module.h"
#include "script/legacy-module.h"

#include "script/engine.h"
#include "script/scope.h"
#include "script/script.h"

#include "script/compiler/compiler.h"

#include "script/private/engine_p.h"
#include "script/private/namespace_p.h"
#include "script/private/script_p.h"

#include <algorithm>

namespace script
{

/*! 
 * \class ModuleInterface
 */

/*!
 * \fn ModuleInterface(Engine* e, std::string name)
 * \brief constructs a module with the given name
 */
ModuleInterface::ModuleInterface(Engine* e, std::string name)
  : m_engine(e),
    m_name(std::move(name))
{

}

/*! 
 * \fn ~ModuleInterface()
 * \brief destroys the module
 */
ModuleInterface::~ModuleInterface()
{

}

/*!
 * \fn virtual void unload()
 * \brief unloads the module
 * 
 * This function should be reimplemented in a subclass if necessary.
 * The default implementation does nothing.
 */
void ModuleInterface::unload()
{

}

/*!
 * \fn virtual Script get_script() const
 * \brief returns the script associated with this module
 *
 * The default implementation returns an invalid script.
 */
Script ModuleInterface::get_script() const
{
  return {};
}

/*!
 * \fn virtual const std::vector<Module>& child_modules() const
 * \brief returns the module's child
 *
 * The default implementation returns an empty vector.
 */
const std::vector<Module>& ModuleInterface::child_modules() const
{
  static const std::vector<Module> static_child_modules = {};
  return static_child_modules;
}

/*!
 * \fn virtual void add_child(Module m)
 * \brief adds a child module to this module
 *
 * The default implementation does nothing.
 */
void ModuleInterface::add_child(Module m)
{

}

/*!
 * \fn void attach(Namespace& ns)
 * \brief attach a namespace to this module
 *
 * This will make the module easily retrievable from the namespace.
 * This function must not be called from the constructor.
 */
void ModuleInterface::attach(Namespace& ns)
{
  ns.impl()->the_module = shared_from_this();
}

/*!
 * \fn void attach(Script& s)
 * \brief attach a script to this module
 *
 * This will make the module easily retrievable from the script.
 * This function must not be called from the constructor.
 */
void ModuleInterface::attach(Script& s)
{
  s.impl()->the_module = shared_from_this();
}

/*!
 * \fn Namespace createRootNamespace()
 * \brief creates a root namespace for this module
 *
 * A namespace created with this function is in no significant way different from 
 * any other namespace; the difference is that it is linked to this module so that 
 * the module can be retrieved from the namespace.
 * 
 * This function must not be called from the constructor.
 */
Namespace ModuleInterface::createRootNamespace()
{
  Namespace ns{ std::make_shared<NamespaceImpl>("", engine()) };
  attach(ns);
  return Namespace(ns);
}


/*!
 * \fn Script createScript(const SourceFile& src)
 * \brief creates a script for this module
 * 
 * This function must not be called from the constructor.
 */
Script ModuleInterface::createScript(const SourceFile& src)
{
  Script s = engine()->newScript(src);
  attach(s);
  return s;
}

/*!
 * \fn void compile(Script& s)
 * \brief compiles the script
 * 
 * Depending on whether a script is currently being compiled, this 
 * function will either compile the script immediately or add it to 
 * the current compile session.
 */
void ModuleInterface::compile(Script& s)
{
  if (!engine()->compiler()->hasActiveSession())
  {
    bool result = s.compile();

    if (!result)
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
  else
  {
    engine()->compiler()->addToSession(s);
  }
}

/*!
 * \endclass
 */

 /*!
  * \class GroupModule
  */

GroupModule::GroupModule(Engine* e, std::string name)
  : ModuleInterface(e, std::move(name))
{

}

bool GroupModule::is_loaded() const
{
  return std::all_of(m_modules.begin(), m_modules.end(), [](const Module& m) -> bool {
    return m.isLoaded();
    });
}

void GroupModule::load()
{
  m_namespace = createRootNamespace();

  for (Module m : child_modules())
    m.load();
}

void GroupModule::unload()
{
  // @TODO: there is no unload() in the Module class
  /*for (Module m : child_modules())
    m.unload();*/
}

Namespace GroupModule::get_global_namespace() const
{
  return m_namespace;
}

const std::vector<Module>& GroupModule::child_modules() const
{
  return m_modules;
}

void GroupModule::add_child(Module m)
{
  m_modules.push_back(m);
}

/*!
 * \endclass
 */

 /*!
  * \class ScriptModule
  */

ScriptModule::ScriptModule(Engine* e, std::string name, const Script& s)
  : ModuleInterface(e, std::move(name))
{
  m_script = s;
}

bool ScriptModule::is_loaded() const
{
  return m_loaded;
}

void ScriptModule::load()
{
  attach(m_script);
  compile(m_script);
  m_loaded = true;
}

void ScriptModule::unload()
{
  // @TODO: correctly clear when the method is implemented
  //m_script.clear();
}

Script ScriptModule::get_script() const
{
  return m_script;
}

Namespace ScriptModule::get_global_namespace() const
{
  return get_script().rootNamespace();
}

/*!
 * \endclass
 */

 /*!
  * \class LegacyModule
  */

LegacyModule::LegacyModule(Engine* e, std::string name, ModuleLoadFunction loadfunc, ModuleCleanupFunction cleanfunc)
  : ModuleInterface(e, std::move(name)),
    m_load(loadfunc),
    m_cleanup(cleanfunc)
{

}

bool LegacyModule::is_loaded() const
{
  return m_loaded;
}

void LegacyModule::load()
{
  m_namespace = createRootNamespace();

  for (Module m : child_modules())
    m.load();

  if (m_load)
    m_load(Module(shared_from_this()));

  m_loaded = true;
}

void LegacyModule::unload()
{
  if (m_cleanup)
    m_cleanup(Module(shared_from_this()));

  m_namespace = Namespace();
  m_loaded = false;
}

Namespace LegacyModule::get_global_namespace() const
{
  return m_namespace;
}

const std::vector<Module>& LegacyModule::child_modules() const
{
  return m_modules;
}

void LegacyModule::add_child(Module m)
{
  m_modules.push_back(m);
}

/*!
 * \endclass
 */


ModuleLoadingError::ModuleLoadingError(std::string mssg)
  : message(std::move(mssg))
{

}

const char* ModuleLoadingError::what() const noexcept
{
  return message.c_str();
}

/*!
 * \class Module
 * \brief Provides module features
 */

Module::Module(std::shared_ptr<ModuleInterface> impl)
  : d(std::move(impl)) 
{

}

/*!
 * \fn Engine* engine() const
 * \brief Returns the engine that was used to construct this module
 */
Engine* Module::engine() const
{
  return d->engine();
}

/*!
 * \fn const std::string& name() const
 * \brief Returns the module name
 */
const std::string & Module::name() const
{
  return d->name();
}

/*!
 * \fn bool isNative() const
 * \brief Returns whether this module is a "native" module.
 * 
 * A "native" module is not implemented by a script.
 */
bool Module::isNative() const
{
  return d->get_script().isNull();
}

/*!
 * \fn Module newSubModule(const std::string& name)
 * \brief Adds a submodule to this module.
 * 
 * This function creates a submodule that defines no symbol but 
 * in which submodule can be added.
 * Use this function if you want to group modules in a common parent module.
 */
Module Module::newSubModule(const std::string & name)
{
  auto mimpl = std::make_shared<GroupModule>(engine(), name);
  Module m{ mimpl };
  addSubModule(m);
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
  auto mimpl = std::make_shared<LegacyModule>(engine(), name, load, cleanup);
  Module m{ mimpl };
  addSubModule(m);
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
  auto mimpl = std::make_shared<ScriptModule>(engine(), name, engine()->newScript(src));
  Module m{ mimpl };
  addSubModule(m);
  return m;
}

/*!
 * \fn Module getSubModule(const std::string& name) const
 * \brief Retrieves an existing submodule by name.
 */
Module Module::getSubModule(const std::string & name) const
{
  for (const auto & child : submodules())
  {
    if (child.name() == name)
      return child;
  }

  return Module{};
}

void Module::addSubModule(Module submodule)
{
  d->add_child(submodule);
}

/*!
 * \fn const std::vector<Module>& submodules() const
 * \brief Returns the module' submodules.
 */
const std::vector<Module> & Module::submodules() const
{
  return d->child_modules();
}

/*!
 * \fn bool isLoaded() const
 * \brief Returns whether the module is loaded.
 */
bool Module::isLoaded() const
{
  return d && d->is_loaded();
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

  d->load();
}

/*!
 * \fn Namespace root() const
 * \brief Returns the module's root namespace
 */
Namespace Module::root() const
{
  return d->get_global_namespace();
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
  Scope scp{ Namespace{ d->get_global_namespace() } };

  Script s = d->get_script();

  if (!s.isNull() && !s.exports().isNull())
    scp.merge(s.exports());

  for (const auto& child : submodules())
  {
    if (!child.isLoaded())
      continue;

    Scope submodule_scope = child.scope();
    scp.merge(submodule_scope);
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
  return d->get_script();
}

void Module::destroy()
{
  d->unload();
}

/*!
 * \endclass
 */

} // namespace script