// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_P_H
#define LIBSCRIPT_CLASS_P_H

#include "script/class.h"
#include "script/function.h"
#include "script/cast.h"
#include "script/template.h"
#include "script/typedefs.h"

#include <map>

namespace script
{

class ScriptImpl;
class UserData;

class ClassImpl
{
public:
  int id;
  std::string name;
  std::weak_ptr<ClassImpl> parent;
  bool isFinal;
  bool isAbstract;
  Engine *engine;
  std::weak_ptr<ScriptImpl> script;
  Function defaultConstructor;
  Function copyConstructor;
  Function moveConstructor;
  Function destructor;
  std::vector<Function> constructors;
  std::vector<Function> functions;
  std::vector<Class> classes;
  std::vector<Enum> enums;
  std::vector<Operator> operators;
  std::vector<Cast> casts;
  std::vector<Template> templates;
  std::vector<Typedef> typedefs;
  std::map<std::string, Class::StaticDataMember> staticMembers;
  std::vector<Class::DataMember> dataMembers;
  std::vector<Function> virtualMembers;
  std::shared_ptr<UserData> data;

  ClassImpl(int i, const std::string & n, Engine *e)
    : id(i)
    , name(n)
    , engine(e)
    , isFinal(false)
    , isAbstract(false)
  {

  }

  Value add_static_data_member(const std::string & name, const Type & t);
  void registerConstructor(const Function & f);

  void set_parent(const Class & p);

  static bool check_overrides(const Function & derived, const Function & base);
  void check_still_abstract();
  void update_vtable(Function f);
  void register_function(const Function & f);
};

} // namespace script

#endif // LIBSCRIPT_CLASS_P_H
