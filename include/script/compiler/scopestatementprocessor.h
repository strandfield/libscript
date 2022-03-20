// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_SCOPE_STATEMENT_PROCESSOR_H
#define LIBSCRIPT_COMPILER_SCOPE_STATEMENT_PROCESSOR_H

#include "script/namelookup.h"
#include "script/namespacealias.h"
#include "script/scope.h"

#include "script/ast/node.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/nameresolver.h"

#include <algorithm>

namespace script
{

namespace compiler
{

class ScopeStatementProcessor
{
public:
  Scope *scope_;

public:
  ScopeStatementProcessor() :
    scope_(nullptr) { }

  void processUsingDirective(const std::shared_ptr<ast::UsingDirective>& decl);
  void processUsingDeclaration(const std::shared_ptr<ast::UsingDeclaration>& decl);
  void processNamespaceAlias(const std::shared_ptr<ast::NamespaceAliasDefinition>& decl);
  void processTypeAlias(const std::shared_ptr<ast::TypeAliasDeclaration>& decl);
  void process(const std::shared_ptr<ast::Statement>& s);

};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_SCOPE_STATEMENT_PROCESSOR_H
