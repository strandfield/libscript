// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/lambda.h"
#include "script/private/lambda_p.h"

namespace script
{

ClosureTypeImpl::ClosureTypeImpl(int i)
  : id(i)
{

}


ClosureType::ClosureType(const std::shared_ptr<ClosureTypeImpl> & impl)
  : d(impl)
{

}

int ClosureType::id() const
{
  return d->id;
}

int ClosureType::captureCount() const
{
  return d->captures.size();
}

bool ClosureType::isCaptureless() const
{
  return d->captures.size() == 0;
}

const std::vector<ClosureType::Capture> ClosureType::captures() const
{
  return d->captures;
}

const Prototype & ClosureType::prototype() const
{
  return d->function.prototype();
}

Operator ClosureType::function() const
{
  return d->function;
}

Engine* ClosureType::engine() const
{
  return d->function.engine();
}

const std::shared_ptr<ClosureTypeImpl> & ClosureType::impl() const
{
  return d;
}

bool ClosureType::operator==(const ClosureType & other) const
{
  return d == other.d;
}

bool ClosureType::operator!=(const ClosureType & other) const
{
  return d != other.d;
}


Lambda::Lambda(const std::shared_ptr<LambdaImpl> & impl)
  : d(impl)
{

}

bool Lambda::isNull() const
{
  return d == nullptr;
}

ClosureType Lambda::closureType() const
{
  return d->closureType;
}

int Lambda::captureCount() const
{
  return d->captures.size();
}

Value Lambda::getCapture(int index) const
{
  return d->captures.at(index);
}

const std::vector<Value> & Lambda::captures() const
{
  return d->captures;
}

Engine* Lambda::engine() const
{
  return d->closureType.engine();
}

const std::shared_ptr<LambdaImpl> & Lambda::impl() const
{
  return d;
}

bool Lambda::operator==(const Lambda & other) const
{
  return d == other.d;
}

} // namespace script
