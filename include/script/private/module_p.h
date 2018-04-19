// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_P_H
#define LIBSCRIPT_MODULE_P_H

#include "script/module.h"

namespace script
{

class ModuleImpl
{
public:
  std::string name;
  Namespace ns;
  ModuleLoadFunction load;
  ModuleCleanupFunction cleanup;
  std::vector<Module> modules;
  bool loaded;

public:
  ModuleImpl(const std::string & module_name, const Namespace & nmspc);
  ModuleImpl(const std::string & module_name, const Namespace & nmspc, ModuleLoadFunction load_callback, ModuleCleanupFunction cleanup_callback);
};

} // namespace script

#endif // LIBSCRIPT_SUPPORT_FILESYSTEM_H
