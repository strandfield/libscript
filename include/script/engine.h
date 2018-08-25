// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENGINE_H
#define LIBSCRIPT_ENGINE_H

#include <vector>

#include "script/scope.h"
#include "script/string.h"
#include "script/types.h"

#include "script/modulecallbacks.h"

#include "script/support/filesystem.h"

#if defined(LIBSCRIPT_CONFIG_ENGINE_INJECTED_HEADERS)
#include LIBSCRIPT_CONFIG_ENGINE_INJECTED_HEADERS
#endif // defined(LIBSCRIPT_CONFIG_ENGINE_INJECTED_HEADERS)

namespace script
{

class Array;
class Class;
class ClassTemplate;
class ClosureType;
class Context;
class ConversionSequence;
class EngineImpl;
class Enum;
class FunctionBuilder;
class FunctionType;
class Namespace;
class Prototype;
class Script;
class SourceFile;
class TemplateArgumentDeduction;
class TemplateParameter;
class Value;

namespace program
{
class Expression;
} // namespace program


class LIBSCRIPT_API Engine
{
public:
  Engine();
  ~Engine();
  Engine(const Engine & other) = delete;

  void setup();

  Value newBool(bool bval);
  Value newChar(char cval);
  Value newInt(int ival);
  Value newFloat(float fval);
  Value newDouble(double dval);
  Value newString(const String & sval);
  
  struct ArrayType { Type type; };
  struct ElementType { Type type; };
  struct fail_if_not_instantiated_t {};
  static const fail_if_not_instantiated_t FailIfNotInstantiated;
  Array newArray(ArrayType t);
  Array newArray(ElementType t);
  Array newArray(ElementType t, fail_if_not_instantiated_t);

  Value construct(Type t, const std::vector<Value> & args);

  Value uninitialized(const Type & t);
  void initialize(Value & memory);
  void uninitialized_copy(const Value & value, Value & memory);
  void emplace(Value & memory, Function ctor, const std::vector<Value> & args);

  void destroy(Value val);

  void manage(Value val);
  void garbageCollect();

  bool canCopy(const Type & t);
  Value copy(const Value & val);

  ConversionSequence conversion(const Type & src, const Type & dest);
  ConversionSequence conversion(const std::shared_ptr<program::Expression> & expr, const Type & dest);

  void applyConversions(std::vector<script::Value> & values, const std::vector<Type> & types, const std::vector<ConversionSequence> & conversions);

  bool canCast(const Type & srcType, const Type & destType);
  Value cast(const Value & val, const Type & type);

  Namespace rootNamespace() const;

  FunctionType getFunctionType(Type id) const;
  FunctionType getFunctionType(const Prototype & proto);

  bool hasType(const Type & t) const;
  Class getClass(Type id) const;
  Enum getEnum(Type id) const;
  ClosureType getLambda(Type id) const;

  void reserveTypeRange(int begin, int end);

  FunctionType newFunctionType(const Prototype & proto);

  Script newScript(const SourceFile & source);
  bool compile(Script s);
  void destroy(Script s);

  Module newModule(const std::string & name);
  Module newModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup);
  const std::vector<Module> & modules() const;
  Module getModule(const std::string & name);

  const std::string & scriptExtension() const;
  void setScriptExtension(const std::string & ex);
  const support::filesystem::path & searchDirectory() const;
  void setSearchDirectory(const support::filesystem::path & dir);

  // Returns the enclosing namespace of the type
  Namespace enclosingNamespace(Type t) const;

  Type typeId(const std::string & typeName, const Scope & scope = Scope()) const;
  std::string typeName(Type t) const;

  Context newContext();
  Context currentContext() const;
  void setContext(Context con);

  /// TODO : add a way to get error messages on failure
  Value eval(const std::string & command, const Scope & scp = Scope{});

  Value call(const Function & f, std::initializer_list<Value> && args);
  Value call(const Function & f, const std::vector<Value> & args);
  Value invoke(const Function & f, std::initializer_list<Value> && args);
  Value invoke(const Function & f, const std::vector<Value> & args);


  struct array_template_t {};
  static const array_template_t ArrayTemplate;
  ClassTemplate getTemplate(array_template_t) const;

  struct initializer_list_template_t {};
  static const initializer_list_template_t InitializerListTemplate;
  ClassTemplate getTemplate(initializer_list_template_t) const;
  bool isInitializerListType(const Type & t) const;


  const std::vector<Script> & scripts() const;

  EngineImpl * implementation() const;

  Engine & operator=(const Engine & other) = delete;

#if defined(LIBSCRIPT_CONFIG_ENGINE_INJECTED_METHODS)
#include LIBSCRIPT_CONFIG_ENGINE_INJECTED_METHODS
#endif // defined(LIBSCRIPT_CONFIG_ENGINE_INJECTED_METHODS)

protected:
  Value buildValue(Type t);

protected:
  std::unique_ptr<EngineImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_ENGINE_H
