// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENGINE_P_H
#define LIBSCRIPT_ENGINE_P_H

#include <map>
#include <typeindex>
#include <vector>

#include "script/enum.h"
#include "script/class.h"
#include "script/classtemplate.h"
#include "script/context.h"
#include "script/module.h"
#include "script/namespace.h"
#include "script/value.h"

#include "script/interpreter/interpreter.h"

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

  std::unique_ptr<TypeSystem> typesystem;

  std::unique_ptr<compiler::Compiler> compiler;
  std::unique_ptr<interpreter::Interpreter> interpreter;

  Context context;
  std::vector<Context> allContexts;

  Namespace rootNamespace;

  std::vector<Script> scripts;
  std::vector<Module> modules;

  struct
  {
    ClassTemplate array;
    ClassTemplate initializer_list;
    std::map<std::type_index, Template> dict;
  }templates;

public:
  /// TODO: move elsewhere, perhaps a namespace 'optimisation'
  Value default_construct(const Type & t, const Function & ctor);
  Value copy(const Value & val, const Function & copyctor);
  void destroy(const Value & val, const Function & dtor);

  void destroy(Namespace ns);
  void destroy(Script s);
};


Value fundamental_conversion(const Value & src, int destType, Engine *e);

} // script

#endif // LIBSCRIPT_ENGINE_P_H
