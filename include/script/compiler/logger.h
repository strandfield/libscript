// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_LOGGER_H
#define LIBSCRIPT_COMPILER_LOGGER_H

#include "script/diagnosticmessage.h"

namespace script
{

namespace compiler
{

class CompilerException;

class LIBSCRIPT_API Logger
{
public:
  virtual ~Logger();

  virtual void log(const diagnostic::Message & mssg);
  virtual void log(const CompilerException & exception);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_LOGGER_H
