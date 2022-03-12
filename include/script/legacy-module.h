// Copyright (C) 2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LEGACYMODULE_H
#define LIBSCRIPT_LEGACYMODULE_H

#include "script/module-interface.h"
#include "script/modulecallbacks.h"

#include "script/module.h"
#include "script/namespace.h"

namespace script
{

/*! 
 * \class LegacyModule
 * \brief a module that defines no symbol but supports submodules
 */

class LIBSCRIPT_API LegacyModule : public ModuleInterface
{
private:
  Namespace m_namespace;
  std::vector<Module> m_modules;
  bool m_loaded = false;
  ModuleLoadFunction m_load = nullptr;
  ModuleCleanupFunction m_cleanup = nullptr;

public:
  LegacyModule(Engine* e, std::string name, ModuleLoadFunction loadfunc, ModuleCleanupFunction cleanfunc);

  bool is_loaded() const override;
  void load() override;
  void unload() override;
  Namespace get_global_namespace() const override;

  const std::vector<Module>& child_modules() const override;
  void add_child(Module m) override;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_LEGACYMODULE_H
