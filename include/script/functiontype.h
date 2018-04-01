// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TYPE_H
#define LIBSCRIPT_FUNCTION_TYPE_H

#include "script/prototype.h"
#include "script/operator.h"

namespace script
{

class FunctionType
{
private:
  Type mType;
  Prototype mPrototype;
  Operator mAssignment;

public:
  FunctionType() = default;
  FunctionType(const FunctionType &) = default;
  ~FunctionType() = default;
  FunctionType(const Type & t, const Prototype & pt, const Operator & op) : mType(t), mPrototype(pt), mAssignment(op) { }

  inline Type type() const { return mType; }
  inline const Prototype & prototype() const { return mPrototype; }
  inline const Operator & assignment() const { return mAssignment; }
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_TYPE_H
