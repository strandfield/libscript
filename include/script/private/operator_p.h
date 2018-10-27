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
public:
  OperatorImpl(OperatorName op, Engine *engine, FunctionImpl::flag_type flags = 0);
  ~OperatorImpl() = default;

  Name get_name() const override;
};

class UnaryOperatorImpl : public OperatorImpl
{
public:
  UnaryOperatorPrototype proto_;

public:
  UnaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags = 0);
  ~UnaryOperatorImpl() = default;

  const Prototype & prototype() const override;
  void set_return_type(const Type & t) override;
};

class BinaryOperatorImpl : public OperatorImpl
{
public:
  BinaryOperatorPrototype proto_;

public:
  BinaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags = 0);
  ~BinaryOperatorImpl() = default;

  const Prototype & prototype() const override;
  void set_return_type(const Type & t) override;
};

class FunctionCallOperatorImpl : public OperatorImpl
{
public:
  DynamicPrototype proto_;

public:
  FunctionCallOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags = 0);
  FunctionCallOperatorImpl(OperatorName op, DynamicPrototype && proto, Engine *engine, FunctionImpl::flag_type flags = 0);
  ~FunctionCallOperatorImpl() = default;

  const Prototype & prototype() const override;
  void set_return_type(const Type & t) override;
};

} // namespace script


#endif // LIBSCRIPT_OPERATOR_P_H
