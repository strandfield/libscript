// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENGINE_H
#define LIBSCRIPT_ENGINE_H

#include <vector>

#include "script/conversions.h"
#include "script/lambda.h"
#include "script/scope.h"
#include "script/script.h"
#include "script/string.h"
#include "script/value.h"

namespace script
{

class Class;
class ClassTemplate;
class Context;
class EngineImpl;
class Enum;
class FunctionBuilder;
class FunctionType;
class FunctionTemplate;
class Namespace;
struct TemplateArgument;

typedef Class(*NativeClassTemplateInstantiationFunction)(ClassTemplate, const std::vector<TemplateArgument> &);
typedef bool(*NativeTemplateDeductionFunction)(std::vector<TemplateArgument> &, const std::vector<Type> &);
typedef Function(*NativeFunctionTemplateInstantiationFunction)(FunctionTemplate, const std::vector<TemplateArgument> &);

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
  
  Array newArray(const Type & valueType);

  Value construct(Type t, const std::vector<Value> & args);

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

  Class getClass(Type id) const;
  Enum getEnum(Type id) const;
  Lambda getLambda(Type id) const;

  FunctionType newFunctionType(const Prototype & proto);
  Enum newEnum(const std::string & name);
  Class newClass(const ClassBuilder &opts);
  Function newFunction(const FunctionBuilder & opts);

  Script newScript(const SourceFile & source);
  bool compile(Script s);

  // Returns the scope in which the class is declared
  Scope scope(const Class & cla);
  Scope scope(const Enum & e);
  Scope scope(const Namespace & n);
  Scope scope(Type type);

  Type typeId(const std::string & typeName, const Scope & scope = Scope()) const;
  std::string typeName(Type t) const;

  Context newContext();
  Context currentContext() const;
  void setContext(Context con);

  /// TODO : add a way to get error messages on failure
  Value eval(const std::string & command, const Script & s = Script{});

  Value call(const Function & f, std::initializer_list<Value> && args);
  Value call(const Function & f, const std::vector<Value> & args);
  Value invoke(const Function & f, std::initializer_list<Value> && args);
  Value invoke(const Function & f, const std::vector<Value> & args);

  FunctionTemplate newFunctionTemplate(const std::string & name, NativeTemplateDeductionFunction deduc, NativeFunctionTemplateInstantiationFunction callback);

  ClassTemplate newClassTemplate(const std::string & name, NativeClassTemplateInstantiationFunction callback);
  struct array_template_t {};
  static const array_template_t ArrayTemplate;
  ClassTemplate getTemplate(array_template_t) const;


  const std::vector<Script> & scripts() const;

  EngineImpl * implementation() const;

  Engine & operator=(const Engine & other) = delete;

protected:
  Value buildValue(Type t);
  Class buildClass(const ClassBuilder & opts, int id = -1);

protected:
  std::unique_ptr<EngineImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_ENGINE_H
