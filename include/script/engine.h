// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENGINE_H
#define LIBSCRIPT_ENGINE_H

#include <vector>

#include "script/exception.h"
#include "script/scope.h"
#include "script/string.h"
#include "script/types.h"
#include "script/value.h"
#include "script/thisobject.h"

#include "script/modulecallbacks.h"

namespace script
{

class Array;
class Class;
class ClassTemplate;
class ClosureType;
class Context;
class Conversion;
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

namespace compiler
{
class Compiler;
} // namespace compiler

namespace program
{
class Expression;
} // namespace program

// base class for all engine exception
class EngineError : public Exception 
{ 
public:
  ErrorCode code_;
  
  EngineError(ErrorCode c) : code_(c) {}
  ErrorCode code() const override { return code_; }
};

// errors returned by Engine::construct 
struct ConstructionError : EngineError { using EngineError::EngineError; };
struct NoMatchingConstructor : ConstructionError { NoMatchingConstructor() : ConstructionError(ErrorCode::E_NoMatchingConstructor) {} };
struct ConstructorIsDeleted : ConstructionError { ConstructorIsDeleted() : ConstructionError(ErrorCode::E_ConstructorIsDeleted) {} };
struct TooManyArgumentInInitialization : ConstructionError { TooManyArgumentInInitialization() : ConstructionError(ErrorCode::E_TooManyArgumentInInitialization) {} };
struct TooFewArgumentInInitialization : ConstructionError { TooFewArgumentInInitialization() : ConstructionError(ErrorCode::E_TooFewArgumentInInitialization) {} };

// error returned by Engine::copy
struct CopyError : EngineError { CopyError() : EngineError(ErrorCode::E_CopyError) {} };

// error returned by Engine::cast
struct ConversionError : EngineError { ConversionError() : EngineError(ErrorCode::E_ConversionError) {} };

// error returned by Engine::typeId
struct UnknownTypeError : EngineError { UnknownTypeError() : EngineError(ErrorCode::E_UnknownTypeError) {} };

// error returned by Engine::eval
struct EvaluationError : EngineError 
{ 
  std::string message;
  EvaluationError(const std::string & mssg) : EngineError(ErrorCode::E_EvaluationError), message(mssg) {} 
};


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
  
  /// TODO: remove (depecated)
  template<typename InitFunc>
  Value construct(Type t, InitFunc && f)
  {
    Value ret = allocate(t);
    f(ret);
    return ret;
  }

  template<typename T, typename...Args>
  Value construct(Args&& ... args)
  {
    Value ret = allocate(Type::make<T>());
    ThisObject self{ ret };
    self.init<T>(std::forward<Args>(args)...);
    return ret;
  }

  void destroy(Value val);

  template<typename T>
  void destroy(Value val)
  {
    ThisObject self{ val };
    self.destroy<T>();
    free(val);
  }

  void manage(Value val);
  void garbageCollect();

  Value allocate(const Type & t);
  void free(Value & v);

  bool canCopy(const Type & t);
  Value copy(const Value & val);

  Conversion conversion(const Type & src, const Type & dest);

  void applyConversions(std::vector<script::Value> & values, const std::vector<Conversion> & conversions);

  bool canCast(const Type & srcType, const Type & destType);
  Value cast(const Value & val, const Type & type);

  Namespace rootNamespace() const;

  FunctionType getFunctionType(Type id) const;
  FunctionType getFunctionType(const Prototype & proto);

  bool hasType(const Type & t) const;
  Class getClass(Type id) const;
  Enum getEnum(Type id) const;
  ClosureType getLambda(Type id) const;

  bool reserveTypeRange(int begin, int end);

  FunctionType newFunctionType(const Prototype & proto);

  Script newScript(const SourceFile & source);
  compiler::Compiler* compiler() const;
  bool compile(Script s);
  void destroy(Script s);

  Module newModule(const std::string & name);
  Module newModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup);
  Module newModule(const std::string & name, const SourceFile & src);
  const std::vector<Module> & modules() const;
  Module getModule(const std::string & name);

  // Returns the enclosing namespace of the type
  Namespace enclosingNamespace(Type t) const;

  Type typeId(const std::string & typeName, Scope scope = Scope()) const;
  std::string typeName(Type t) const;

  Context newContext();
  Context currentContext() const;
  void setContext(Context con);

  /// TODO : add a way to get error messages on failure
  Value eval(const std::string & command);

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

protected:
  std::unique_ptr<EngineImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_ENGINE_H
