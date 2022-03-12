// Copyright (C) 2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_INTERFACE_H
#define LIBSCRIPT_MODULE_INTERFACE_H

#include "libscriptdefs.h"

#include <string>
#include <vector>

namespace script
{

class Engine;
class Namespace;
class Module;
class Script;
class SourceFile;

/*! 
 * \class ModuleInterface
 * \brief abstract base class for all modules
 */

class LIBSCRIPT_API ModuleInterface : public std::enable_shared_from_this<ModuleInterface>
{
private:
  Engine* m_engine;
  std::string m_name;

public:
  ModuleInterface(Engine* e, std::string name);
  virtual ~ModuleInterface();

  Engine* engine() const;

  const std::string& name() const;

  /*!
   * \fn virtual bool is_loaded() const = 0
   * \brief returns whether the module is loaded
   */
  virtual bool is_loaded() const = 0;

  /*!
   * \fn virtual void load()  = 0
   * \brief loads the module
   */
  virtual void load() = 0;

  virtual void unload();

  virtual Script get_script() const;

  /*!
   * \fn virtual Namespace get_global_namespace() const = 0
   * \brief returns the root of the module's symbol tree 
   */
  virtual Namespace get_global_namespace() const = 0;

  virtual const std::vector<Module>& child_modules() const;
  virtual void add_child(Module m);

protected:
  void attach(Namespace& ns);
  void attach(Script& s);
  Namespace createRootNamespace();
  Script createScript(const SourceFile& src);
  void compile(Script& s);
};


/*!
 * \fn Engine* engine() const
 * \brief returns a pointer to the engine
 */
inline Engine* ModuleInterface::engine() const
{
  return m_engine;
}

/*!
 * \fn const std::string& name() const
 * \brief returns the module's name
 */
inline const std::string& ModuleInterface::name() const
{
  return m_name;
}

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_MODULE_INTERFACE_H
