// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LAMBDA_P_H
#define LIBSCRIPT_LAMBDA_P_H

#include "script/lambda.h"

#include "script/operator.h"

namespace script
{

class LambdaImpl
{
public:
  int id;
  std::vector<Lambda::Capture> captures;
  Operator function;

public:
  LambdaImpl(int id);
};

class LambdaObjectImpl
{
public:
  Lambda closureType;
  std::vector<Value> captures;

public:
  LambdaObjectImpl(const Lambda & l)
    : closureType(l)
  {
    captures.reserve(closureType.captureCount());
  }
};

} // namespace script

#endif // LIBSCRIPT_LAMBDA_P_H
