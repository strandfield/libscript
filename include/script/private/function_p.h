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
class Expression;
class Statement;
}

class Class;
class Name;
class SymbolImpl;

typedef std::shared_ptr<program::Expression> DefaultArgument;

class DefaultArgumentList
{
public:
  DefaultArgumentList();
  DefaultArgumentList(const DefaultArgumentList &) = delete;
  ~DefaultArgumentList();

  bool isEmpty() const;
  size_t size() const;
  void push_back(const DefaultArgument & value);

  void set(std::vector<DefaultArgument> && defargs);
  std::vector<DefaultArgument> & get();
  const std::vector<DefaultArgument> & get() const;

private:
  std::unique_ptr<std::vector<DefaultArgument>> data;
};

static_assert(sizeof(DefaultArgumentList) == sizeof(std::unique_ptr<std::vector<DefaultArgument>>), "haa");

class LIBSCRIPT_API FunctionImpl
{
public:
  typedef int flag_type;

  FunctionImpl(const Prototype &p, Engine *e, flag_type f = 0);
  virtual ~FunctionImpl();

  virtual const std::string & name() const;
  virtual Name get_name() const;

  Prototype prototype;
  Engine *engine;
  std::shared_ptr<UserData> data;
  std::weak_ptr<SymbolImpl> enclosing_symbol;
  flag_type flags;
  struct {
    NativeFunctionSignature callback;
    std::shared_ptr<program::Statement> program;
  }implementation;
  DefaultArgumentList default_arguments;

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
  const std::string & name() const override
  {
    return mName;
  }

  Name get_name() const override;
};

class ScriptFunctionImpl : public FunctionImpl
{
public:
  ScriptFunctionImpl(Engine *e);
  ~ScriptFunctionImpl() = default;
};

class ConstructorImpl : public FunctionImpl
{
public:
  ConstructorImpl(const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
public:
  
  Class getClass() const;

  const std::string & name() const override;
  Name get_name() const override;

  bool is_default_ctor() const;
  bool is_copy_ctor() const;
  bool is_move_ctor() const;
};


class DestructorImpl : public FunctionImpl
{
public:
  DestructorImpl(const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
};


class FunctionTemplateInstance : public RegularFunctionImpl
{
public:
  FunctionTemplateInstance(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const std::string & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
  ~FunctionTemplateInstance() = default;

  FunctionTemplate mTemplate;
  std::vector<TemplateArgument> mArgs;

  static std::shared_ptr<FunctionTemplateInstance> create(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const FunctionBuilder & builder);

};

} // namespace script


#endif // LIBSCRIPT_FUNCTION_P_H
