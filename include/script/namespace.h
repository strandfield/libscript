// Copyright (C) 2018 Vincent Chambrin
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
class Function;
class FunctionBuilder;
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

  Enum newEnum(const std::string & name, int id = -1);
  Function newFunction(const FunctionBuilder & opts);
  Class newClass(const ClassBuilder & opts);
  Namespace newNamespace(const std::string & name);
  Operator newOperator(const FunctionBuilder & opts);
  void addValue(const std::string & name, const Value & val);
  void addOperator(const Operator & op);
  [[deprecated("Use TemplateBuilder instead")]] void addTemplate(const Template & t);

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
  FunctionBuilder Operation(OperatorName op, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder UserDefinedLiteral(const std::string & suffix, NativeFunctionSignature func = nullptr) const;
  FunctionBuilder UserDefinedLiteral(const std::string & suffix, const Type & input, const Type & output, NativeFunctionSignature func = nullptr) const;

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
