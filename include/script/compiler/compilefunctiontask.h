// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_FUNCTION_TASK_H
#define LIBSCRIPT_COMPILE_FUNCTION_TASK_H

#include "script/function.h"
#include "script/scope.h"

#include "script/ast/forwards.h"

namespace script
{

namespace compiler
{

struct CompileFunctionTask
{
  CompileFunctionTask() { }
  CompileFunctionTask(const Function & f, const std::shared_ptr<ast::FunctionDecl> & d, const Scope & s) :
    function(f), declaration(d), scope(s) { }

  Function function;
  std::shared_ptr<ast::FunctionDecl> declaration;
  Scope scope;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_FUNCTION_TASK_H
