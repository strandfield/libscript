// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMESPACE_H
#define LIBSCRIPT_NAMESPACE_H

#include <map>

#include "class.h"
#include "operator.h"

namespace script
{

class NamespaceImpl;

class FunctionBuilder;
class Module;
class Template;
class Typedef;

class LIBSCRIPT_API Namespace
{
public:
  Namespace() = default;
  Namespace(const Namespace & other) = default;
  ~Namespace() = default;

  Namespace(const std::shared_ptr<NamespaceImpl> & impl);

  bool isNull() const;
  bool isRoot() const;
  bool isUnnamed() const;

  bool isScriptNamespace() const;
  Script asScript() const;

  bool isModuleNamesapce() const;
  Module asModule() const;

  const std::string & name() const;

  Enum newEnum(const std::string & name);
  Function newFunction(const FunctionBuilder & opts);
  Class newClass(const ClassBuilder & opts);
  Namespace newNamespace(const std::string & name);
  Operator newOperator(const FunctionBuilder & opts);
  void addValue(const std::string & name, const Value & val);
  void addOperator(const Operator & op);
  void addTemplate(const Template & t);

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

  FunctionBuilder Function(const std::string & name, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder Operation(Operator::BuiltInOperator op, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder UserDefinedLiteral(const std::string & suffix, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder UserDefinedLiteral(const std::string & suffix, const Type & input, const Type & output, NativeFunctionSignature func = nullptr) const;

  Engine * engine() const;
  NamespaceImpl * implementation() const;
  std::weak_ptr<NamespaceImpl> weakref() const;

  Namespace & operator=(const Namespace &) = default;
  bool operator==(const Namespace & other) const;
  bool operator!=(const Namespace & other) const;

private:
  std::shared_ptr<NamespaceImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_NAMESPACE_H
