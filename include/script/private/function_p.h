// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_P_H
#define LIBSCRIPT_FUNCTION_P_H

#include "script/engine.h"
#include "script/functionflags.h"
#include "script/functiontemplate.h"
#include "script/prototypes.h"
#include "script/private/symbol_p.h"

namespace script
{

namespace program
{
class Expression;
class Statement;
}

class Class;
class Name;

typedef std::shared_ptr<program::Expression> DefaultArgument;

class LIBSCRIPT_API FunctionImpl : public SymbolImpl
{
public:
  FunctionImpl(Engine *e, FunctionFlags f = FunctionFlags{});
  ~FunctionImpl();

  virtual const std::string& name() const;
  virtual const std::string& literal_operator_suffix() const;
  Name get_name() const override;
  bool is_function() const override;

  bool is_ctor() const;
  bool is_dtor() const;

  FunctionFlags flags;
  std::shared_ptr<UserData> data;

  virtual bool is_native() const = 0;
  virtual std::shared_ptr<program::Statement> body() const;
  virtual void set_body(std::shared_ptr<program::Statement> b) = 0;

  virtual const Prototype& prototype() const = 0;
  virtual void set_return_type(const Type& t);

  virtual script::Value invoke(script::FunctionCall* c);

  void force_virtual();

  virtual bool is_template_instance() const;
  virtual bool is_instantiation_completed() const;
  virtual void complete_instantiation();
  
  virtual const std::vector<DefaultArgument> & default_arguments() const;
  virtual void set_default_arguments(std::vector<DefaultArgument> defaults);
};

class RegularFunctionImpl : public FunctionImpl
{
public:
  RegularFunctionImpl(std::string name, const Prototype& p, Engine *e, FunctionFlags f = FunctionFlags{});
  RegularFunctionImpl(std::string name, DynamicPrototype p, Engine *e, FunctionFlags f = FunctionFlags{});

  std::string mName;
  DynamicPrototype prototype_;
  std::shared_ptr<program::Statement> program_;
  std::vector<DefaultArgument> mDefaultArguments;
public:
  const std::string& name() const override;
  SymbolKind get_kind() const override;
  Name get_name() const override;

  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;

  const Prototype& prototype() const override;
  void set_return_type(const Type& t) override;

  const std::vector<DefaultArgument>& default_arguments() const override;
  void set_default_arguments(std::vector<DefaultArgument> defaults) override;
};

class ScriptFunctionImpl : public FunctionImpl
{
public:
  DynamicPrototype prototype_;
  std::shared_ptr<program::Statement> program_;

public:
  ScriptFunctionImpl(Engine *e);
  ~ScriptFunctionImpl() = default;

  SymbolKind get_kind() const override;
  const std::string& name() const override;
  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;
  const Prototype& prototype() const override;
};

// @TODO: remove dllexport
class LIBSCRIPT_API ConstructorImpl : public FunctionImpl
{
public:
  ConstructorImpl(const Prototype& p, Engine *e, FunctionFlags f = FunctionFlags{});
  
  DynamicPrototype prototype_;
  std::shared_ptr<program::Statement> program_;
  std::vector<DefaultArgument> mDefaultArguments;
public:
  
  Class getClass() const;

  const std::string& name() const override;
  SymbolKind get_kind() const override;
  Name get_name() const override;

  const Prototype& prototype() const override;

  const std::vector<DefaultArgument>& default_arguments() const override;
  void set_default_arguments(std::vector<DefaultArgument> defaults) override;

  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;
};


class DestructorImpl : public FunctionImpl
{
public:
  DestructorPrototype proto_;
  std::shared_ptr<program::Statement> program_;

public:
  DestructorImpl(const Prototype &p, Engine *e, FunctionFlags f = FunctionFlags{});

  SymbolKind get_kind() const override;

  const Prototype& prototype() const override;

  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;
};


class FunctionTemplateInstance : public RegularFunctionImpl
{
public:
  FunctionTemplateInstance(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const std::string & name, const Prototype &p, Engine *e, FunctionFlags f = FunctionFlags{});
  ~FunctionTemplateInstance() = default;

  FunctionTemplate mTemplate;
  std::vector<TemplateArgument> mArgs;

  static std::shared_ptr<FunctionTemplateInstance> create(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const FunctionBlueprint& blueprint);

  bool is_template_instance() const override;
  bool is_instantiation_completed() const override;
  void complete_instantiation() override;
};

} // namespace script


#endif // LIBSCRIPT_FUNCTION_P_H
