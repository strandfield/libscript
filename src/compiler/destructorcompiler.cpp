// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/destructorcompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/expressioncompiler.h"

#include "script/ast/node.h"

#include "script/program/statements.h"

#include "script/class.h"
#include "script/datamember.h"
#include "script/engine.h"
#include "script/namelookup.h"
#include "script/overloadresolution.h"
#include "script/typesystem.h"

namespace script
{

namespace compiler
{

DestructorCompiler::DestructorCompiler(FunctionCompiler *c)
  : FunctionCompilerExtension(c)
{
  assert(c != nullptr);
}

std::shared_ptr<program::CompoundStatement> DestructorCompiler::generateFooter()
{
  return generateDestructor(currentClass());
}

std::shared_ptr<program::CompoundStatement> DestructorCompiler::generateDestructor(const Class & cla)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  const auto & data_members = cla.dataMembers();
  for (size_t i(data_members.size()); i-- > 0; )
    statements.push_back(program::PopDataMember::New(getDestructor(cla.engine(), data_members.at(i).type)));

  if (!cla.parent().isNull())
  {
    /// TODO : check if dtor exists and is not deleted
    auto this_object = program::StackValue::New(1, Type::ref(cla.id()));
    std::vector<std::shared_ptr<program::Expression>> args{ this_object };
    auto dtor_call = program::FunctionCall::New(cla.parent().destructor(), std::move(args));
    statements.push_back(program::ExpressionStatement::New(dtor_call));
  }

  return program::CompoundStatement::New(std::move(statements));
}


Function DestructorCompiler::getDestructor(Engine *e, const Type & t)
{
  if (t.isObjectType())
    return e->typeSystem()->getClass(t).destructor();
  return Function{};
}

} // namespace compiler

} // namespace script

