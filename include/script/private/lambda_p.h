// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LAMBDA_P_H
#define LIBSCRIPT_LAMBDA_P_H

#include "script/lambda.h"

#include "script/operator.h"
#include "script/private/class_p.h"

namespace script
{

class ClosureTypeImpl : public ClassImpl
{
public:
  std::vector<ClosureType::Capture> captures;

public:
  ClosureTypeImpl(int id, Engine *e);
};

class LambdaImpl
{
public:
  ClosureType closureType;
  std::vector<Value> captures;

public:
  LambdaImpl(const ClosureType & l)
    : closureType(l)
  {
    captures.reserve(closureType.captureCount());
  }
};

} // namespace script

#endif // LIBSCRIPT_LAMBDA_P_H
