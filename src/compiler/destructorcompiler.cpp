// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/destructorcompiler.h"
#include "script/compiler/compilererrors.h"

#include "script/ast/node.h"

#include "script/program/statements.h"

#include "script/namelookup.h"
#include "script/overloadresolution.h"

namespace script
{

namespace compiler
{

DestructorCompiler::DestructorCompiler(FunctionCompiler *c)
  : compiler(c)
{
  assert(c != nullptr);
}

std::shared_ptr<program::CompoundStatement> DestructorCompiler::generateFooter()
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  const auto & data_members = compiler->classScope().dataMembers();
  for (int i(data_members.size() - 1); i >= 0; --i)
    statements.push_back(program::PopDataMember::New(getDestructor(data_members.at(i).type)));

  if (!compiler->classScope().parent().isNull())
  {
    /// TODO : check if dtor exists and is not deleted
    auto this_object = compiler->generateThisAccess();
    std::vector<std::shared_ptr<program::Expression>> args{ this_object };
    auto dtor_call = program::FunctionCall::New(compiler->classScope().parent().destructor(), std::move(args));
    statements.push_back(program::ExpressionStatement::New(dtor_call));
  }

  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> DestructorCompiler::generateDestructor()
{
  return generateFooter();
}


Function DestructorCompiler::getDestructor(const Type & t)
{
  if (t.isObjectType())
    return compiler->engine()->getClass(t).destructor();
  return Function{};
}

} // namespace compiler

} // namespace script

