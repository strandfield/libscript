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

class CompilationFailure : public Exceptional
{
public:
  SourceLocation location;

public:

  CompilationFailure(CompilationFailure&&) noexcept = default;

  explicit CompilationFailure(CompilerError e)
    : Exceptional(e)
  {

  }

  template<typename T>
  CompilationFailure(CompilerError e, T&& d)
    : Exceptional(e, std::forward<T>(d))
  {

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
  std::string name;
  std::string message;
};

} // namespace errors

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_COMPILERERRORS_H
