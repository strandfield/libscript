// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/functionprocessor.h"

#include "script/compiler/typeresolver.h"

namespace script
{

namespace compiler
{

static Class getClass(const Scope& scp)
{
  return scp.isClass() ? scp.asClass() : (scp.type() == Scope::TemplateArgumentScope ? getClass(scp.parent()) : Class{});
}

/*!
 * \fn void fill_prototype(FunctionBuilder& builder, const std::shared_ptr<ast::FunctionDecl>& fundecl, const Scope& scp)
 * \brief fills the prototype field of a function builder
 * \param the function builder to fill
 * \param the function declaration in the ast
 * \param the scope in which name are resolved
 */
void fill_prototype(FunctionBuilder& builder, const std::shared_ptr<ast::FunctionDecl>& fundecl, const Scope& scp)
{
  const Class class_scope = getClass(scp);

  if (!fundecl->is<ast::ConstructorDecl>() && !fundecl->is<ast::DestructorDecl>())
    builder.returns(script::compiler::resolve_type(fundecl->returnType, scp));

  for (size_t i(0); i < fundecl->params.size(); ++i)
  {
    Type argtype = script::compiler::resolve_type(fundecl->params.at(i).type, scp);
    builder.params(argtype);
  }
}

} // namespace compiler

} // namespace script

