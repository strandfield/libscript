// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LAMBDA_H
#define LIBSCRIPT_LAMBDA_H

#include "script/value.h"
#include "script/class.h"

namespace script
{

class LambdaImpl;

class LIBSCRIPT_API Lambda
{
public:
  Lambda() = default;
  Lambda(const Lambda &) = default;
  ~Lambda() = default;

  explicit Lambda(const std::shared_ptr<LambdaImpl> & impl);

  struct Capture {
    Type type;
    std::string name;
  };

  int id() const;

  int captureCount() const;
  bool isCaptureless() const;
  const std::vector<Capture> captures() const;

  const Prototype & prototype() const;
  Operator function() const;

  Engine* engine() const;

  const std::shared_ptr<LambdaImpl> & impl() const;

  Lambda & operator=(const Lambda &) = default;
  bool operator==(const Lambda & other) const;
  bool operator!=(const Lambda & other) const;

private:
  std::shared_ptr<LambdaImpl> d;
};


class LambdaObjectImpl;

class LIBSCRIPT_API LambdaObject
{
public:
  LambdaObject() = default;
  LambdaObject(const LambdaObject &) = default;
  ~LambdaObject() = default;

  LambdaObject(const std::shared_ptr<LambdaObjectImpl> & impl);

  bool isNull() const;
  Lambda closureType() const;

  int captureCount() const;
  Value getCapture(int index) const;
  const std::vector<Value> & captures() const;

  Engine* engine() const;

  const std::shared_ptr<LambdaObjectImpl> & impl() const;

  LambdaObject & operator=(const LambdaObject & ) = default;
  bool operator==(const LambdaObject & other) const;
  inline bool operator!=(const LambdaObject & other) const { return !operator==(other); }

private:
  std::shared_ptr<LambdaObjectImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_LAMBDA_H
