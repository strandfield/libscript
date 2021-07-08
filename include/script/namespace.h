// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMESPACE_H
#define LIBSCRIPT_NAMESPACE_H

#include "libscriptdefs.h"
#include "script/callbacks.h"
#include "script/operators.h"

#include <map>
#include <vector>

namespace script
{

class NamespaceImpl;

class Class;
class ClassBuilder;
class Engine;
class Enum;
class EnumBuilder;
class Function;
class LiteralOperator;
class Module;
class Operator;
class Script;
class Template;
class Type;
class Typedef;
class Value;

class LIBSCRIPT_API Namespace
{
public:
  Namespace() = default;
  Namespace(const Namespace & other) = default;
  ~Namespace() = default;

  explicit Namespace(const std::shared_ptr<NamespaceImpl> & impl);

  bool isNull() const;
  bool isRoot() const;
  bool isUnnamed() const;

  bool isScriptNamespace() const;
  Script asScript() const;
  Script script() const;

  bool isModuleNamespace() const;
  Module asModule() const;

  const std::string & name() const;

  Namespace getNamespace(const std::string & name);

  Namespace newNamespace(const std::string & name);
  void addValue(const std::string & name, const Value & val);

  const std::map<std::string, Value> & vars() const;
  const std::vector<Enum> & enums() const;
  const std::vector<Function> & functions() const;
  const std::vector<Operator> & operators() const;
  const std::vector<LiteralOperator> & literalOperators() const;
  const std::vector<Class> & classes() const;
  const std::vector<Namespace> & namespaces() const;
  const std::vector<Template> & templates() const;
  const std::vector<Typedef> & typedefs() const;

  Namespace enclosingNamespace() const;

  Class findClass(const std::string & name) const;
  Namespace findNamespace(const std::string & name) const;
  std::vector<Function> findFunctions(const std::string & name) const;

  ClassBuilder newClass(const std::string & name) const;
  EnumBuilder newEnum(const std::string & name) const;

  void addFunction(const Function& f);

  Engine * engine() const;
  inline const std::shared_ptr<NamespaceImpl> & impl() const { return d; }

  Namespace & operator=(const Namespace &) = default;
  bool operator==(const Namespace & other) const;
  bool operator!=(const Namespace & other) const;

private:
  std::shared_ptr<NamespaceImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_NAMESPACE_H
