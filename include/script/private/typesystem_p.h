// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPESYSTEM_P_H
#define LIBSCRIPT_TYPESYSTEM_P_H

#include <vector>

#include "script/enum.h"
#include "script/class.h"
#include "script/classtemplate.h"
#include "script/functiontype.h"
#include "script/context.h"
#include "script/module.h"
#include "script/namespace.h"
#include "script/value.h"

#include "script/interpreter/interpreter.h"

namespace script
{

class Engine;

class TypeSystemImpl
{
public:
  TypeSystemImpl(Engine *e);
  TypeSystemImpl(const TypeSystemImpl&) = delete;
  ~TypeSystemImpl() = default;

  static std::unique_ptr<TypeSystemImpl> create(Engine* e);

public:
  Engine *engine;

  std::vector<FunctionType> prototypes;
  std::vector<Class> classes;
  std::vector<Enum> enums;
  std::vector<ClosureType> lambdas;

  struct {
    Enum enum_type;
    Class class_type;
  } reservations;

public:
  void reserveTypes();

  ClosureType newLambda();

  void register_class(Class & c, int id = 0);
  void register_enum(Enum & e, int id = 0);

  void destroy(Enum e);
  void destroy(Class c);

  void unregister_class(Class &c);
  void unregister_enum(Enum &e);
  void unregister_closure(ClosureType &c);

private:
  void reserveTypes(int begin, int end);
};

} // script

#endif // LIBSCRIPT_TYPESYSTEM_P_H
