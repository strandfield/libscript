// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_P_H
#define LIBSCRIPT_FUNCTION_P_H

#include "script/engine.h"
#include "script/functionflags.h"
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
  // @TODO: replace by a virtual function & move this field to RegularFunctionImpl
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
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_P_H
