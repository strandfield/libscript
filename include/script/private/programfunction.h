// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PROGRAMFUNCTION_H
#define LIBSCRIPT_PROGRAMFUNCTION_H

#include "script/private/function_p.h"
#include "script/functiontemplate.h"

namespace script
{

class ProgramFunction : public FunctionImpl
{
public:
  ProgramFunction(Engine* e, FunctionFlags f = FunctionFlags{});

  std::shared_ptr<program::Statement> program_;
  std::shared_ptr<UserData> data_;

public:
  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;

  std::shared_ptr<UserData> get_user_data() const override;
  void set_user_data(std::shared_ptr<UserData> d) override;
};

class RegularFunctionImpl : public ProgramFunction
{
public:
  RegularFunctionImpl(std::string name, const Prototype& p, Engine *e, FunctionFlags f = FunctionFlags{});
  RegularFunctionImpl(std::string name, DynamicPrototype p, Engine *e, FunctionFlags f = FunctionFlags{});

  std::string mName;
  DynamicPrototype prototype_;

public:
  const std::string& name() const override;
  SymbolKind get_kind() const override;
  Name get_name() const override;

  const Prototype& prototype() const override;
  void set_return_type(const Type& t) override;
};

class ScriptFunctionImpl : public ProgramFunction
{
public:
  DynamicPrototype prototype_;

public:
  ScriptFunctionImpl(Engine *e);
  ~ScriptFunctionImpl() = default;

  SymbolKind get_kind() const override;
  const std::string& name() const override;
  const Prototype& prototype() const override;
};

class ConstructorImpl : public ProgramFunction
{
public:
  ConstructorImpl(const Prototype& p, Engine *e, FunctionFlags f = FunctionFlags{});
  
  DynamicPrototype prototype_;

public:
  
  Class getClass() const;

  const std::string& name() const override;
  SymbolKind get_kind() const override;
  Name get_name() const override;

  const Prototype& prototype() const override;
};


class DestructorImpl : public ProgramFunction
{
public:
  DestructorPrototype proto_;

public:
  DestructorImpl(const Prototype &p, Engine *e, FunctionFlags f = FunctionFlags{});

  SymbolKind get_kind() const override;

  const Prototype& prototype() const override;
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


#endif // LIBSCRIPT_PROGRAMFUNCTION_H
