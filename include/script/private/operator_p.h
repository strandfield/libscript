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
  OperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags = 0);
  OperatorImpl(OperatorName op, DynamicPrototype && proto, Engine *engine, FunctionImpl::flag_type flags = 0);
  ~OperatorImpl() = default;

  Name get_name() const override;
};

class BuiltInOperatorImpl : public OperatorImpl
{
public:
  BuiltInOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine);
  BuiltInOperatorImpl(OperatorName op, DynamicPrototype && proto, Engine *engine);
  ~BuiltInOperatorImpl() = default;
};

} // namespace script


#endif // LIBSCRIPT_OPERATOR_P_H
