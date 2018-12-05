// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_TEMPLATE_DEFINITION_H
#define LIBSCRIPT_COMPILER_TEMPLATE_DEFINITION_H

#include "script/ast/ast_p.h"

namespace script
{

class Scope;

namespace compiler
{

class LIBSCRIPT_API TemplateDefinition
{
public:
  size_t line_offset_;
  size_t source_offset_;
  std::shared_ptr<ast::AST> ast_;
  std::shared_ptr<ast::TemplateDeclaration> decl_;

  TemplateDefinition() = default;
  TemplateDefinition(const TemplateDefinition &) = default;
  ~TemplateDefinition() = default;

  std::shared_ptr<ast::ClassDecl> get_class_decl() const;
  std::shared_ptr<ast::FunctionDecl> get_function_decl() const;

  static TemplateDefinition make(const std::shared_ptr<ast::TemplateDeclaration> & decl);

  TemplateDefinition & operator=(const TemplateDefinition &) = default;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_TEMPLATE_DEFINITION_H
