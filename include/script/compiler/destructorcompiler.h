// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DESTRUCTOR_COMPILER_H
#define LIBSCRIPT_DESTRUCTOR_COMPILER_H

#include "script/compiler/functioncompiler.h"

namespace script
{

class OverloadResolution;

namespace compiler
{

class DestructorCompiler
{
private:
  FunctionCompiler *compiler;
public:
  explicit DestructorCompiler(FunctionCompiler *c);

  std::shared_ptr<program::CompoundStatement> generateFooter();

  std::shared_ptr<program::CompoundStatement> generateDestructor();

protected:
  Function getDestructor(const Type & t);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_DESTRUCTOR_COMPILER_H
