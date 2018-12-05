// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/templatedefinition.h"

#include "script/ast/visitor.h"

#include "script/script.h"
#include "script/private/script_p.h"

namespace script
{

namespace compiler
{

std::shared_ptr<ast::ClassDecl> TemplateDefinition::get_class_decl() const
{
  return std::static_pointer_cast<ast::ClassDecl>(decl_->declaration);
}

std::shared_ptr<ast::FunctionDecl> TemplateDefinition::get_function_decl() const
{
  return std::static_pointer_cast<ast::FunctionDecl>(decl_->declaration);
}

TemplateDefinition TemplateDefinition::make(Script s, const std::shared_ptr<ast::TemplateDeclaration> & decl)
{
  TemplateDefinition ret;

  //ret.ast_ = decl->ast.lock();
  s.impl()->astlock = true;

  ret.decl_ = decl;
  
  return ret;
}

} // namespace compiler

} // namespace script

