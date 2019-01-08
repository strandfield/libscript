// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_H
#define LIBSCRIPT_MODULE_H

#include "script/modulecallbacks.h"

#include <string>
#include <vector>

namespace script
{

class Engine;
class Namespace;
class Scope;
class Script;

class NamespaceImpl;

class LIBSCRIPT_API Module
{
public:
  Module() = default;
  Module(const Module & other) = default;
  ~Module() = default;

  explicit Module(const std::shared_ptr<NamespaceImpl> & impl);

  inline bool isNull() const { return d == nullptr; }
  Engine * engine() const;

  const std::string & name() const;

  bool isNative() const;

  Module newSubModule(const std::string & name);
  Module newSubModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup);
  Module getSubModule(const std::string & name) const;
  const std::vector<Module> & submodules() const;

  bool isLoaded() const;
  void load();

  Namespace root() const;
  Scope scope() const;

  Script asScript() const;

  inline NamespaceImpl* impl() const { return d.get(); }
  inline std::weak_ptr<NamespaceImpl> weakref() const { return std::weak_ptr<NamespaceImpl>(d); }
  inline const std::shared_ptr<NamespaceImpl> & strongref() const { return d; }

private:
  friend class Engine;
  void destroy();

private:
  std::shared_ptr<NamespaceImpl> d;
};

} // namespace script


#endif // LIBSCRIPT_CONTEXT_H
