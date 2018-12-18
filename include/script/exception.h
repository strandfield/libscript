// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_EXCEPTION_H
#define LIBSCRIPT_EXCEPTION_H

#include "script/errorcodes.h"

#include <string>

namespace script
{

class Exception
{
public:
  Exception() = default;
  Exception(const Exception &) = default;
  virtual ~Exception() = default;

  virtual ErrorCode code() const = 0;

  template<typename T>
  const T & as() const { return *dynamic_cast<const T*>(this); }

  template<typename T>
  bool is() const { return dynamic_cast<const T*>(this) != nullptr; }

  Exception & operator=(const Exception &) = delete;
};

template<ErrorCode EC>
class GenericException : public Exception
{
public:
  ErrorCode code() const override { return EC; }
};

class NotImplemented : public GenericException<ErrorCode::NotImplementedError>
{
public:
  std::string message;

  NotImplemented(std::string && mssg) : message(std::move(mssg)) {}
};

class RuntimeError : public Exception
{
public:
  /// TODO: add StackTrace

  ErrorCode code() const override { return ErrorCode::RuntimeError; }
};

} // namespace script

#endif // LIBSCRIPT_EXCEPTION_H
