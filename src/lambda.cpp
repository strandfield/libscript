// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/lambda.h"
#include "lambda_p.h"

namespace script
{

LambdaImpl::LambdaImpl(int i)
  : id(i)
{

}


Lambda::Lambda(const std::shared_ptr<LambdaImpl> & impl)
  : d(impl)
{

}

int Lambda::id() const
{
  return d->id;
}

int Lambda::captureCount() const
{
  return d->captures.size();
}

bool Lambda::isCaptureless() const
{
  return d->captures.size() == 0;
}

const std::vector<Lambda::Capture> Lambda::captures() const
{
  return d->captures;
}

const Prototype & Lambda::prototype() const
{
  return d->function.prototype();
}

Operator Lambda::function() const
{
  return d->function;
}

Engine* Lambda::engine() const
{
  return d->function.engine();
}

const std::shared_ptr<LambdaImpl> & Lambda::impl() const
{
  return d;
}

bool Lambda::operator==(const Lambda & other) const
{
  return d == other.d;
}

bool Lambda::operator!=(const Lambda & other) const
{
  return d != other.d;
}


LambdaObject::LambdaObject(const std::shared_ptr<LambdaObjectImpl> & impl)
  : d(impl)
{

}

bool LambdaObject::isNull() const
{
  return d == nullptr;
}

Lambda LambdaObject::closureType() const
{
  return d->closureType;
}

int LambdaObject::captureCount() const
{
  return d->captures.size();
}

Value LambdaObject::getCapture(int index) const
{
  return d->captures.at(index);
}

const std::vector<Value> & LambdaObject::captures() const
{
  return d->captures;
}

Engine* LambdaObject::engine() const
{
  return d->closureType.engine();
}

const std::shared_ptr<LambdaObjectImpl> & LambdaObject::impl() const
{
  return d;
}

bool LambdaObject::operator==(const LambdaObject & other) const
{
  return d == other.d;
}

} // namespace script
