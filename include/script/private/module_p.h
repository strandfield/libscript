// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_P_H
#define LIBSCRIPT_MODULE_P_H

#include "script/module.h"
#include "script/private/namespace_p.h"

namespace script
{

class ModuleImpl : public NamespaceImpl
{
public:
  ModuleLoadFunction load;
  ModuleCleanupFunction cleanup;
  std::vector<Module> modules;
  bool loaded;

public:
  ModuleImpl(Engine *e, const std::string & module_name);
  ModuleImpl(Engine *e, const std::string & module_name, ModuleLoadFunction load_callback, ModuleCleanupFunction cleanup_callback);
  ~ModuleImpl() = default;
};

} // namespace script

#endif // LIBSCRIPT_MODULE_P_H
