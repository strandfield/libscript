// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_H
#define LIBSCRIPT_MODULE_H

#include "script/exception.h"
#include "script/modulecallbacks.h"

#include <string>
#include <vector>

namespace script
{

class Engine;
class Namespace;
class Scope;
class Script;
class SourceFile;

class ModuleInterface;

struct LIBSCRIPT_API ModuleLoadingError : public std::exception
{
  std::string message;

  ModuleLoadingError(std::string mssg);

  const char* what() const noexcept override;
};

/*!
 * \class Module
 */

class LIBSCRIPT_API Module
{
public:
  Module() = default;
  Module(const Module&) = default;
  ~Module() = default;

  explicit Module(std::shared_ptr<ModuleInterface> impl);

  inline bool isNull() const { return d == nullptr; }
  Engine * engine() const;

  const std::string & name() const;

  bool isNative() const;

  Module newSubModule(const std::string & name);
  Module newSubModule(const std::string& name, ModuleLoadFunction load, ModuleCleanupFunction cleanup);
  Module newSubModule(const std::string& name, const SourceFile& src);
  template<typename T, typename...Args> Module newSubModule(Args&&... args);
  Module getSubModule(const std::string & name) const;
  void addSubModule(Module submodule);
  const std::vector<Module>& submodules() const;

  bool isLoaded() const;
  void load();

  Namespace root() const;
  Scope scope() const;

  Script asScript() const;

  inline ModuleInterface* impl() const { return d.get(); }
  inline std::weak_ptr<ModuleInterface> weakref() const { return std::weak_ptr<ModuleInterface>(d); }
  inline const std::shared_ptr<ModuleInterface> & strongref() const { return d; }

  Module& operator=(const Module&) = default;

private:
  friend class Engine;
  void destroy();

private:
  std::shared_ptr<ModuleInterface> d;
};

/*!
 * \fn Module newSubModule(Args&&... args)
 * \tparam T, the type of the module interface
 * \param the pack of arguments to be forwarded to T constructor
 */
template<typename T, typename...Args>
inline Module Module::newSubModule(Args&&... args)
{
  auto module_impl = std::make_shared<T>(std::forward<Args>(args)...);
  addSubModule(Module(module_impl));
  return Module(module_impl);
}

/*!
 * \endclass
 */

} // namespace script


#endif // LIBSCRIPT_MODULE_H
