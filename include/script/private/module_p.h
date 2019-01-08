// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_P_H
#define LIBSCRIPT_MODULE_P_H

#include "script/module.h"
#include "script/private/script_p.h"

namespace script
{

class NativeModule : public NamespaceImpl
{
public:
  ModuleLoadFunction load;
  ModuleCleanupFunction cleanup;
  std::vector<Module> modules;
  bool loaded;

public:
  NativeModule(Engine *e, const std::string & module_name);
  NativeModule(Engine *e, const std::string & module_name, ModuleLoadFunction load_callback, ModuleCleanupFunction cleanup_callback);
  ~NativeModule() = default;

  bool is_module() const override;
  bool is_native_module() const override;
};

class ScriptModule : public ScriptImpl
{
public:
  ScriptModule(int id, Engine *e, const SourceFile & src, const std::string & module_name);
  ~ScriptModule() = default;

  bool is_module() const override;
};

} // namespace script

#endif // LIBSCRIPT_MODULE_P_H
