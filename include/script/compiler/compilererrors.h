// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_COMPILERERRORS_H
#define LIBSCRIPT_COMPILER_COMPILERERRORS_H

#include "script/compiler/errors.h"

#include "script/accessspecifier.h"
#include "script/diagnosticmessage.h"
#include "script/exception.h"
#include "script/operators.h"
#include "script/types.h"

#include <string>

namespace script
{

namespace compiler
{

class CompilerException : public Exception
{
public:
  diagnostic::pos_t pos;

public:
  CompilerException() : pos{-1, -1} { }
  CompilerException(const CompilerException &) { }
  virtual ~CompilerException() = default;

  CompilerException(const diagnostic::pos_t & p) : pos(p) {}
};


#define CE(Name) public: \
  ~Name() = default; \
  ErrorCode code() const override { return ErrorCode::C_##Name; }


#define GENERIC_COMPILER_EXCEPTION(Name) class Name : public CompilerException \
{ \
public: \
  Name(diagnostic::pos_t p = diagnostic::pos_t{-1,-1}) : CompilerException(p) { } \
  ErrorCode code() const override { return ErrorCode::C_##Name; } \
  ~Name() = default; \
}

#undef CE
#undef GENERIC_COMPILER_EXCEPTION

} // namespace compiler

} // namespace script

namespace script
{

namespace compiler
{

struct CompilerErrorData
{
  virtual ~CompilerErrorData();

  template<typename T>
  T& get();
};

template<typename T>
struct CompilerErrorDataWrapper : CompilerErrorData
{
public:
  T value;

public:
  CompilerErrorDataWrapper(const CompilerErrorDataWrapper<T>&) = delete;
  ~CompilerErrorDataWrapper() = default;

  CompilerErrorDataWrapper(T&& data) : value(std::move(data)) { }
};

template<typename T>
inline T& CompilerErrorData::get()
{
  return static_cast<CompilerErrorDataWrapper<T>*>(this)->value;
}

class CompilationFailure : public Exceptional
{
public:
  SourceLocation location;
  std::unique_ptr<CompilerErrorData> data;

public:

  CompilationFailure(CompilationFailure&&) noexcept = default;

  explicit CompilationFailure(CompilerError e)
    : Exceptional(e)
  {

  }

  template<typename T>
  CompilationFailure(CompilerError e, T&& d)
    : Exceptional(e)
  {
    data = std::make_unique<CompilerErrorDataWrapper<T>>(std::move(d));
  }
};

namespace errors
{

struct InvalidName
{
  std::string name;
};

struct DataMemberName
{
  std::string name;
};

struct VariableType
{
  script::Type type;
};

struct ParameterCount
{
  int actual;
  int expected;
};

struct ConversionFailure
{
  script::Type src;
  script::Type dest;
};

struct NarrowingConversion
{
  script::Type src;
  script::Type dest;
};

struct NoCommonType
{
  script::Type first;
  script::Type second;
};

struct InaccessibleMember
{
  std::string name;
  script::AccessSpecifier access;
};

struct ModuleImportationFailed
{
  std::string message;
};

} // namespace errors

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_COMPILERERRORS_H
