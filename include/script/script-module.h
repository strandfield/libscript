// Copyright (C) 2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCRIPTMODULE_H
#define LIBSCRIPT_SCRIPTMODULE_H

#include "script/module-interface.h"
#include "script/script.h"

namespace script
{

/*! 
 * \class ScriptModule
 * \brief a module that is defined by a script
 */

class LIBSCRIPT_API ScriptModule : public ModuleInterface
{
private:
  Script m_script;
  bool m_loaded = false;

public:
  ScriptModule(Engine* e, std::string name, const Script& s);

  bool is_loaded() const override;
  void load() override;
  void unload() override;
  Script get_script() const override;
  Namespace get_global_namespace() const override;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_SCRIPTMODULE_H
