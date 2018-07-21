// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LAMBDA_H
#define LIBSCRIPT_LAMBDA_H

#include "script/value.h" /// TODO: remove this include
#include "script/class.h"

namespace script
{

class ClosureTypeImpl;
class Prototype;

class LIBSCRIPT_API ClosureType
{
public:
  ClosureType() = default;
  ClosureType(const ClosureType &) = default;
  ~ClosureType() = default;

  explicit ClosureType(const std::shared_ptr<ClosureTypeImpl> & impl);

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

  const std::shared_ptr<ClosureTypeImpl> & impl() const;

  ClosureType & operator=(const ClosureType &) = default;
  bool operator==(const ClosureType & other) const;
  bool operator!=(const ClosureType & other) const;

private:
  std::shared_ptr<ClosureTypeImpl> d;
};


class LambdaImpl;

class LIBSCRIPT_API Lambda
{
public:
  Lambda() = default;
  Lambda(const Lambda &) = default;
  ~Lambda() = default;

  Lambda(const std::shared_ptr<LambdaImpl> & impl);

  bool isNull() const;
  ClosureType closureType() const;

  int captureCount() const;
  Value getCapture(int index) const;
  const std::vector<Value> & captures() const;

  Engine* engine() const;

  const std::shared_ptr<LambdaImpl> & impl() const;

  Lambda & operator=(const Lambda & ) = default;
  bool operator==(const Lambda & other) const;
  inline bool operator!=(const Lambda & other) const { return !operator==(other); }

private:
  std::shared_ptr<LambdaImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_LAMBDA_H
