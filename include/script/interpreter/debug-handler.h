// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_INTERPRETER_DEBUG_HANDLER_H
#define LIBSCRIPT_INTERPRETER_DEBUG_HANDLER_H

#include "libscriptdefs.h"

namespace script
{

namespace program
{
struct Breakpoint;
} // namespace program

namespace interpreter
{

class FunctionCall;

class LIBSCRIPT_API DebugHandler
{
public:
  DebugHandler() = default;
  virtual ~DebugHandler();

  virtual void interrupt(FunctionCall& call, program::Breakpoint& info) = 0;
};

} // namespace interpreter

} // namespace script

#endif // LIBSCRIPT_INTERPRETER_DEBUG_HANDLER_H
