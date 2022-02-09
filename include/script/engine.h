// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENGINE_H
#define LIBSCRIPT_ENGINE_H

#include <map>
#include <string>
#include <typeindex>
#include <vector>

#include "script/compilemode.h"
#include "script/exception.h"
#include "script/scope.h"
#include "script/string.h"
#include "script/types.h"
#include "script/value.h"

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

LIBSCRIPT_API const std::error_category& engine_category() noexcept;

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
  std::string message;

  RuntimeError() : EngineError(EngineError::RuntimeError) { }

  RuntimeError(std::string mssg) : EngineError(EngineError::RuntimeError), message(std::move(mssg)) { }
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
  void tearDown();

  TypeSystem* typeSystem() const;

  template<typename T>
  Type registerType(std::string name);

  template<typename T>
  Type getType() const;

  template<typename T>
  Type makeType() const;

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

  template<typename T, typename...Args>
  Value construct(Args&& ... args)
  {
    return Value(new CppValue<T>(this, std::forward<Args>(args)...));
  }

  void destroy(Value val);

  template<typename T>
  void destroy(Value val)
  {
    /* this is a no-op */
  }

  template<typename T>
  Value expose(T& val)
  {
    return Value(new CppReferenceValue<T>(this, val));
  }

  bool canCopy(const Type & t);
  Value copy(const Value & val);

  bool canConvert(const Type & srcType, const Type & destType) const;
  Value convert(const Value & val, const Type & type);

  Namespace rootNamespace() const;

  Script newScript(const SourceFile & source);
  bool compile(Script s, CompileMode mode = CompileMode::Release);
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
  Type register_type(std::type_index id, Type::TypeFlag what);
  void register_type(std::type_index id, Type t);
  Type find_type_or_throw(std::type_index id) const;

protected:
  std::unique_ptr<EngineImpl> d;
};

/*!
 * \fn template<typename T> Type registerType(std::string name)
 * \brief register a new type
 */
template<typename T>
inline Type Engine::registerType(std::string name)
{
  static_assert(std::is_class<T>::value || std::is_enum<T>::value, "T must be enum or class");
  Type::TypeFlag what = std::is_class<T>::value ? Type::ObjectFlag : Type::EnumFlag;
  return register_type(std::type_index(typeid(T)), what);
}

/*!
 * \fn template<typename T> Type getType() const
 * \brief get a type
 * 
 * Throws \t UnknownTypeError if the type wasn't previously registered
 * with \m registerType.
 */
template<typename T>
inline Type Engine::getType() const
{
  static const script::Type cache = find_type_or_throw(std::type_index(typeid(T)));
  return cache;
}

template<typename T>
struct maketype_helper
{
  inline static Type get(const Engine& e)
  {
    return e.getType<T>();
  }
};

template<typename T>
struct maketype_helper<T&>
{
  inline static Type get(const Engine& e)
  {
    return Type::ref(maketype_helper<T>::get(e));
  }
};

template<typename T>
struct maketype_helper<T&&>
{
  inline static Type get(const Engine& e)
  {
    return Type::rref(maketype_helper<T>::get(e));
  }
};

template<typename T>
struct maketype_helper<const T>
{
  inline static Type get(const Engine& e)
  {
    return maketype_helper<T>::get(e).withFlag(Type::ConstFlag);
  }
};

template<typename T>
struct maketype_helper<const T&>
{
  inline static Type get(const Engine& e)
  {
    return Type::cref(maketype_helper<T>::get(e));
  }
};

template<typename T>
struct maketype_helper<const T*>
{
  inline static Type get(const Engine& e)
  {
    return maketype_helper<T*>::get(e).withFlag(Type::ConstFlag);
  }
};

template<> struct maketype_helper<void> { static Type get(const Engine&) { return Type::Void; } };
template<> struct maketype_helper<bool> { static Type get(const Engine&) { return Type::Boolean; } };
template<> struct maketype_helper<char> { static Type get(const Engine&) { return Type::Char; } };
template<> struct maketype_helper<int> { static Type get(const Engine&) { return Type::Int; } };
template<> struct maketype_helper<float> { static Type get(const Engine&) { return Type::Float; } };
template<> struct maketype_helper<double> { static Type get(const Engine&) { return Type::Double; } };
template<> struct maketype_helper<String> { static Type get(const Engine&) { return Type::String; } };

/*!
 * \fn template<typename T> Type makeType() const
 * \brief constructs a Type from a C++ type
 *
 * This function may use \m getType and as such can raise 
 * an \t UnknownTypeError.
 */
template<typename T>
inline Type Engine::makeType() const
{
  static const script::Type cache = maketype_helper<T>::get(*this);
  return cache;
}


template<> inline Value Engine::construct<bool>(const bool& x) { return newBool(x); }
template<> inline Value Engine::construct<bool>(bool&& x) { return newBool(x); }
template<> inline Value Engine::construct<char>(const char& c) { return newChar(c); }
template<> inline Value Engine::construct<char>(char&& c) { return newChar(c); }
template<> inline Value Engine::construct<int>(const int& n) { return newInt(n); }
template<> inline Value Engine::construct<int>(int&& n) { return newInt(n); }
template<> inline Value Engine::construct<float>(const float& n) { return newFloat(n); }
template<> inline Value Engine::construct<float>(float&& n) { return newFloat(n); }
template<> inline Value Engine::construct<double>(const double& n) { return newDouble(n); }
template<> inline Value Engine::construct<double>(double&& n) { return newDouble(n); }
template<> inline Value Engine::construct<String>(const String& s) { return newString(s); }
template<> inline Value Engine::construct<String>(String&& s) { return newString(s); }

} // namespace script

namespace std
{

template<> struct is_error_code_enum<script::EngineError::ErrorCode> : std::true_type { };

} // namespace std

#endif // LIBSCRIPT_ENGINE_H
