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

/*!
 * \class DebugHandler
 * 
 * This class can be used to implement a debugger server within the Interpreter.
 * 
 * You can use the virtual \m interrupt() method to pause the execution of 
 * the program and implement very common debugging functionalities.
 * 
 * Step-by-step: break once at the closest breakpoint in the same file.
 * 
 * Step into: break once at the next possible breakpoint
 * 
 */
class LIBSCRIPT_API DebugHandler
{
public:
  DebugHandler() = default;
  virtual ~DebugHandler();

  /*!
   * \fn void interrupt(FunctionCall& call, program::Breakpoint& info)
   * \brief provides an entry point for tracking the execution of a program
   * 
   * This function is called at every possible breakpoint in the program when 
   * the script is compiled in debug mode.
   */
  virtual void interrupt(FunctionCall& call, program::Breakpoint& info) = 0;
};

/*!
 * \endclass
 */

} // namespace interpreter

} // namespace script

#endif // LIBSCRIPT_INTERPRETER_DEBUG_HANDLER_H
