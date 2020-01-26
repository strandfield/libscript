// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENGINE_H
#define LIBSCRIPT_ENGINE_H

#include <map>
#include <string>
#include <typeindex>
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
class TypeSystem;

namespace compiler
{
class Compiler;
} // namespace compiler

namespace interpreter
{
class Interpreter;
} // namespace interpreter

namespace program
{
class Expression;
} // namespace program

namespace errors
{

const std::error_category& engine_category() noexcept;

} // namespace errors

// base class for all engine exception
class LIBSCRIPT_API EngineError : public Exceptional 
{ 
public:
  
  enum ErrorCode {
    NotImplemented = 1,
    RuntimeError = 2,
    EvaluationError,
    ConversionError,
    CopyError,
    UnknownType,
    NoMatchingConstructor,
    ConstructorIsDeleted,
    TooManyArgumentInInitialization,
    TooFewArgumentInInitialization,
  };

  explicit EngineError(ErrorCode ec);
};

// errors returned by Engine::construct 
struct LIBSCRIPT_API ConstructionError : EngineError { using EngineError::EngineError; };

// error returned by Engine::copy
struct LIBSCRIPT_API CopyError : EngineError { CopyError() : EngineError(EngineError::CopyError) {} };

// error returned by Engine::cast
struct LIBSCRIPT_API ConversionError : EngineError { ConversionError() : EngineError(EngineError::ConversionError) {} };

// error returned by Engine::typeId
struct LIBSCRIPT_API UnknownTypeError : EngineError { UnknownTypeError() : EngineError(ErrorCode::UnknownType) {} };

// error returned by Engine::eval
struct LIBSCRIPT_API EvaluationError : EngineError
{ 
  std::string message;
  EvaluationError(const std::string & mssg) : EngineError(EngineError::EvaluationError), message(mssg) {}
};

struct LIBSCRIPT_API NotImplemented : public EngineError
{
public:
  std::string message;

  NotImplemented(std::string&& mssg) : EngineError(EngineError::NotImplemented), message(std::move(mssg)) {}
};

struct LIBSCRIPT_API RuntimeError : public EngineError
{
public:
  /// TODO: add StackTrace

  RuntimeError() : EngineError(EngineError::RuntimeError) { }
};

inline std::error_code make_error_code(script::EngineError::ErrorCode e) noexcept
{
  return std::error_code(static_cast<int>(e), script::errors::engine_category());
}


class LIBSCRIPT_API Engine
{
public:
  Engine();
  ~Engine();
  Engine(const Engine & other) = delete;

  void setup();

  TypeSystem* typeSystem() const;

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

  bool canConvert(const Type & srcType, const Type & destType) const;
  Value convert(const Value & val, const Type & type);

  Namespace rootNamespace() const;

  Script newScript(const SourceFile & source);
  bool compile(Script s);
  void destroy(Script s);

  Module newModule(const std::string & name);
  Module newModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup);
  Module newModule(const std::string & name, const SourceFile & src);
  const std::vector<Module> & modules() const;
  Module getModule(const std::string & name);

  Type typeId(const std::string & typeName, Scope scope = Scope()) const;

  std::string toString(const Type& t) const;
  std::string toString(const Function& f) const;

  Context newContext();
  Context currentContext() const;
  void setContext(Context con);

  Value eval(const std::string & command);

  compiler::Compiler* compiler() const;
  interpreter::Interpreter* interpreter() const;

  const std::map<std::type_index, Template>& templateMap() const;

  const std::vector<Script> & scripts() const;

  EngineImpl * implementation() const;

  Engine & operator=(const Engine & other) = delete;

protected:
  std::unique_ptr<EngineImpl> d;
};

} // namespace script

namespace std
{

template<> struct is_error_code_enum<script::EngineError::ErrorCode> : std::true_type { };

} // namespace std

#endif // LIBSCRIPT_ENGINE_H
