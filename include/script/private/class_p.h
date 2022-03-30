// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_P_H
#define LIBSCRIPT_CLASS_P_H

#include "script/private/symbol_p.h"

#include "script/cast.h"
#include "script/class.h"
#include "script/classtemplate.h"
#include "script/datamember.h"
#include "script/enum.h"
#include "script/function.h"
#include "script/operator.h"
#include "script/staticdatamember.h"
#include "script/typedefs.h"

#include <map>

namespace script
{

class NamespaceImpl;
class ScriptImpl;
class UserData;

class ClassImpl : public SymbolImpl
{
public:
  int id;
  std::string name;
  std::weak_ptr<ClassImpl> parent;
  bool isFinal;
  bool isAbstract;
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
  std::vector<Function> friend_functions;
  std::vector<Class> friend_classes;

  ClassImpl(int i, const std::string & n, Engine *e)
    : SymbolImpl(e)
    , id(i)
    , name(n)
    , isFinal(false)
    , isAbstract(false)
  {

  }

  virtual ~ClassImpl() = default;

  Value add_default_constructed_static_data_member(const std::string & name, const Type & t, AccessSpecifier aspec = AccessSpecifier::Public);
  void registerConstructor(const Function & f);

  void set_parent(const Class & p);

  static bool check_overrides(const Function & derived, const Function & base);
  void check_still_abstract();
  void update_vtable(Function f);
  void register_function(const Function & f);

protected:
  Name get_name() const override;
};

class ClassTemplateInstance : public ClassImpl
{
public:
  ClassTemplate instance_of;
  std::vector<TemplateArgument> template_arguments;

public:
  ClassTemplateInstance(ClassTemplate t, const std::vector<TemplateArgument> & args, int i, const std::string & n, Engine *e);
  ~ClassTemplateInstance() = default;
};

} // namespace script

#endif // LIBSCRIPT_CLASS_P_H
