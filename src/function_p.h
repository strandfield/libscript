// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_P_H
#define LIBSCRIPT_FUNCTION_P_H

#include "script/engine.h"
#include "script/prototype.h"
#include "script/functiontemplate.h"

namespace script
{

namespace program
{
class Statement;
}

class LIBSCRIPT_API FunctionImpl
{
public:
  typedef int flag_type;

  FunctionImpl(const Prototype &p, Engine *e, flag_type f = 0);
  virtual ~FunctionImpl();

  virtual std::string name() const = 0;

  Prototype prototype;
  Engine *engine;
  std::shared_ptr<UserData> data;
  std::weak_ptr<ScriptImpl> script;
  flag_type flags;
  struct {
    NativeFunctionSignature callback;
    std::shared_ptr<program::Statement> program;
  }implementation;

  void force_virtual();
  void set_impl(NativeFunctionSignature callback);
  void set_impl(const std::shared_ptr<program::Statement> program);
};

class RegularFunctionImpl : public FunctionImpl
{
public:
  RegularFunctionImpl(const std::string & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
  std::string mName;
public:
  std::string name() const override
  {
    return mName;
  }
};


class ScriptFunctionImpl : public FunctionImpl
{
public:
  ScriptFunctionImpl(Engine *e);
  ~ScriptFunctionImpl() = default;

  std::string name() const override
  {
    throw std::runtime_error{ "Function::name() :  function has no name" };
  }
};

class ConstructorImpl : public FunctionImpl
{
public:
  ConstructorImpl(const Class & cla, const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
  Class mClass;
public:
  std::string name() const override
  {
    return mClass.name();
  }

  bool is_default_ctor() const;
  bool is_copy_ctor() const;
  bool is_move_ctor() const;
};


class DestructorImpl : public FunctionImpl
{
public:
  DestructorImpl(const Class & cla, const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
  Class mClass;
public:
  std::string name() const override
  {
    return mClass.name();
  }
};


class FunctionTemplateInstance : public RegularFunctionImpl
{
public:
  FunctionTemplateInstance(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const std::string & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
  ~FunctionTemplateInstance() = default;

  FunctionTemplate mTemplate;
  std::vector<TemplateArgument> mArgs;

};

} // namespace script


#endif // LIBSCRIPT_FUNCTION_P_H
