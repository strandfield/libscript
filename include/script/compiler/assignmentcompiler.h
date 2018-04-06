// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ASSIGNMENT_COMPILER_H
#define LIBSCRIPT_ASSIGNMENT_COMPILER_H

#include "script/compiler/functioncompiler.h"

namespace script
{

class OverloadResolution;

namespace compiler
{

class AssignmentCompiler
{
private:
  FunctionCompiler *compiler;
public:
  explicit AssignmentCompiler(FunctionCompiler *c);

  std::shared_ptr<program::CompoundStatement> generateAssignmentOperator();

protected:
  static bool isAssignmentOperator(const Operator & op, const Type & t);
  Operator findAssignmentOperator(const Type & t);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_ASSIGNMENT_COMPILER_H
