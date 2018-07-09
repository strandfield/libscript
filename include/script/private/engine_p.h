// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENGINE_P_H
#define LIBSCRIPT_ENGINE_P_H

#include <vector>

#include "script/enum.h"
#include "script/class.h"
#include "script/classtemplate.h"
#include "script/context.h"
#include "script/module.h"
#include "script/namespace.h"
#include "script/value.h"

#include "script/interpreter/interpreter.h"

#include "script/support/filesystem.h"

#if defined(LIBSCRIPT_CONFIG_ENGINE_IMPL_INJECTED_HEADERS)
#include LIBSCRIPT_CONFIG_ENGINE_IMPL_INJECTED_HEADERS
#endif // defined(LIBSCRIPT_CONFIG_ENGINE_IMPL_INJECTED_HEADERS)


namespace script
{

class Engine;

class EngineImpl
{
public:
  EngineImpl(Engine *e);
  EngineImpl(const EngineImpl &) = delete;
  ~EngineImpl() = default;

public:
  Engine *engine;

  std::unique_ptr<interpreter::Interpreter> interpreter;

  Context context;
  std::vector<Context> allContexts;

  Namespace rootNamespace;

  std::vector<Value> garbageCollector;
  bool garbage_collector_running;

  std::vector<FunctionType> prototypes;
  std::vector<Class> classes;
  std::vector<Enum> enums;
  std::vector<ClosureType> lambdas;
  std::vector<Script> scripts;
  std::vector<Module> modules;
  
  std::string script_extension;
  support::filesystem::path search_dir;

  struct {
    Enum enum_type;
    Class class_type;
  } reservations;

  struct
  {
    ClassTemplate array;
  }templates;

public:
  Value buildValue(Type t);

  Object createObject(Type t);
  Object createObject(Class cla);

  Function newConstructor(const FunctionBuilder & opts);
  Function newDestructor(const FunctionBuilder & opts);
  Function newFunction(const FunctionBuilder & opts);
  Function newLiteralOperator(const FunctionBuilder & opts);
  Function newOperator(const FunctionBuilder & opts);
  Function newCast(const FunctionBuilder & opts);

  /// TODO: move elsewhere, perhaps a namespace 'optimisation'
  Value default_construct(const Type & t, const Function & ctor);
  Value copy(const Value & val, const Function & copyctor);
  void destroy(const Value & val, const Function & dtor);

  void placement(const Function & ctor, Value object, const std::vector<Value> & args);

  ClosureType newLambda();

  script::Class new_class(const ClassBuilder & opts);
  void fill_class(std::shared_ptr<ClassImpl> impl, const ClassBuilder & opts);
  void register_class(Class & c, int id = 0);

  void destroyClass(Class c);
  void destroyEnum(Enum e);

#if defined(LIBSCRIPT_CONFIG_ENGINE_IMPL_INJECTED_MEMBERS)
#include LIBSCRIPT_CONFIG_ENGINE_IMPL_INJECTED_MEMBERS
#endif // defined(LIBSCRIPT_CONFIG_ENGINE_IMPL_INJECTED_MEMBERS)

};


Value fundamental_conversion(const Value & src, int destType, Engine *e);

} // script

#endif // LIBSCRIPT_ENGINE_P_H
