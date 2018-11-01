// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DESTRUCTOR_COMPILER_H
#define LIBSCRIPT_DESTRUCTOR_COMPILER_H

#include "script/compiler/functioncompilerextension.h"

namespace script
{

class Function;
class Type;

namespace program
{
class CompoundStatement;
} // namespace program

namespace compiler
{

class DestructorCompiler : public FunctionCompilerExtension
{
public:
  explicit DestructorCompiler(FunctionCompiler *c);

  std::shared_ptr<program::CompoundStatement> generateFooter();

  static std::shared_ptr<program::CompoundStatement> generateDestructor(const Class & cla);

protected:
  static Function getDestructor(Engine *e, const Type & t);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_DESTRUCTOR_COMPILER_H
