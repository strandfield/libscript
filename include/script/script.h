// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCRIPT_H
#define LIBSCRIPT_SCRIPT_H

#include <string>

#include "diagnosticmessage.h"
#include "sourcefile.h"
#include "namespace.h"

namespace script
{

class ScriptImpl;

class Engine;

class LIBSCRIPT_API Script
{
public:
  Script() = default;
  Script(const Script &) = default;
  ~Script() = default;

  Script(const std::shared_ptr<ScriptImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  int id() const;
  const std::string & name() const;
  void setName(const std::string & name);

  bool compile();
  bool isReady() const;
  inline bool isCompiled() const { return isReady(); }
  void run();

  void clear();

  const std::string & Script::path() const;
  const SourceFile & source() const;
  const Namespace & rootNamespace() const;
  const std::map<std::string, int> & globalNames() const;
  const std::vector<Value> & globals() const;

  const std::vector<Class> & classes() const;
  const std::vector<Operator> & operators() const;

  const std::vector<diagnostic::Message> & messages() const;

  Engine * engine() const;
  ScriptImpl * implementation() const;
  std::weak_ptr<ScriptImpl> weakref() const;

  Script & operator=(const Script &) = default;

  bool operator==(const Script & other) const;
  bool operator!=(const Script & other) const;

private:
  std::shared_ptr<ScriptImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_SCRIPT_H
