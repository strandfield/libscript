// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OPERATOR_P_H
#define LIBSCRIPT_OPERATOR_P_H

#include "script/private/function_p.h"

#include "script/operator.h"

namespace script
{

class OperatorImpl : public FunctionImpl
{
public:
  OperatorName operatorId;
  std::shared_ptr<program::Statement> program_;

public:
  OperatorImpl(OperatorName op, Engine *engine, FunctionFlags flags);
  ~OperatorImpl() = default;

  SymbolKind get_kind() const override;
  Name get_name() const override;

  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;
};

class UnaryOperatorImpl : public OperatorImpl
{
public:
  UnaryOperatorPrototype proto_;

public:
  UnaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionFlags flags);
  ~UnaryOperatorImpl() = default;

  const Prototype & prototype() const override;
  void set_return_type(const Type & t) override;
};

class BinaryOperatorImpl : public OperatorImpl
{
public:
  BinaryOperatorPrototype proto_;

public:
  BinaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionFlags flags);
  ~BinaryOperatorImpl() = default;

  const Prototype & prototype() const override;
  void set_return_type(const Type & t) override;
};

class FunctionCallOperatorImpl : public OperatorImpl
{
public:
  DynamicPrototype proto_;
  std::vector<DefaultArgument> defaultargs_;

public:
  FunctionCallOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionFlags flags);
  FunctionCallOperatorImpl(OperatorName op, DynamicPrototype && proto, Engine *engine, FunctionFlags flags);
  ~FunctionCallOperatorImpl() = default;

  const Prototype & prototype() const override;
  void set_return_type(const Type & t) override;

  const std::vector<DefaultArgument> & default_arguments() const override;
  void set_default_arguments(std::vector<DefaultArgument> defaults) override;
};

} // namespace script


#endif // LIBSCRIPT_OPERATOR_P_H
