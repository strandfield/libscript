// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_SCOPED_DECLARATION_H
#define LIBSCRIPT_COMPILER_SCOPED_DECLARATION_H

#include "script/scope.h"

namespace script
{

namespace ast
{
class Declaration;
} // namespace ast

namespace compiler
{

struct ScopedDeclaration
{
  ScopedDeclaration() { }
  ScopedDeclaration(const script::Scope & scp, const std::shared_ptr<ast::Declaration> & decl) : scope(scp), declaration(decl) { }

  Scope scope;
  std::shared_ptr<ast::Declaration> declaration;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_SCOPED_DECLARATION_H
