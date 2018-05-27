// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OPERATOR_P_H
#define LIBSCRIPT_OPERATOR_P_H

#include "function_p.h"

#include "script/operator.h"

namespace script
{

class OperatorImpl : public FunctionImpl
{
public:
  Operator::BuiltInOperator operatorId;
public:
  OperatorImpl(Operator::BuiltInOperator op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags = 0);
  ~OperatorImpl() = default;

  std::string name() const override
  {
    throw std::runtime_error{ "Not implemented" };
  }
};

class BuiltInOperatorImpl : public OperatorImpl
{
public:
  BuiltInOperatorImpl(Operator::BuiltInOperator op, const Prototype & proto, Engine *engine);
  ~BuiltInOperatorImpl() = default;
};

} // namespace script


#endif // LIBSCRIPT_OPERATOR_P_H
