// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPESYSTEM_P_H
#define LIBSCRIPT_TYPESYSTEM_P_H

#include <map>
#include <memory>
#include <typeindex>
#include <vector>

#include "script/enum.h"
#include "script/class.h"
#include "script/classtemplate.h"
#include "script/functiontype.h"
#include "script/context.h"
#include "script/module.h"
#include "script/namespace.h"
#include "script/typesystemlistener.h"
#include "script/value.h"

#include "script/interpreter/interpreter.h"

namespace script
{

class Engine;
class TypeSystemTransaction;

class TypeSystemImpl
{
public:
  TypeSystemImpl(Engine *e);
  TypeSystemImpl(const TypeSystemImpl&) = delete;
  ~TypeSystemImpl();

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

  std::map<std::type_index, Type> typemap;
  std::map<std::string, Type> typemap_by_name;

  std::vector<std::unique_ptr<TypeSystemListener>> listeners;

  TypeSystemTransaction* active_transaction = nullptr;

public:
  void reserveTypes();

  ClosureType newLambda();

  void register_class(Class & c, int id = 0);
  void register_enum(Enum & e, int id = 0);

  void destroy(const Type& t);

  void destroy(Enum e);
  void destroy(Class c);

  void unregister_class(Class &c);
  void unregister_enum(Enum &e);
  void unregister_closure(ClosureType &c);
  void unregister_function(FunctionType& ft);

  void notify_creation(const Type& t);
  void notify_destruction(const Type& t);

private:
  void reserveTypes(int begin, int end);
};

} // script

#endif // LIBSCRIPT_TYPESYSTEM_P_H
