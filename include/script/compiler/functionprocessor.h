// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
#define LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H

#include "script/compiler/component.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/class.h"
#include "script/functionbuilder.h"
#include "script/symbol.h"

#include <vector>

namespace script
{

namespace compiler
{

class PrototypeResolver
{
public:

  inline Class getClass(const Scope & scp)
  {
    return scp.isClass() ? scp.asClass() : (scp.type() == Scope::TemplateArgumentScope ? getClass(scp.parent()) : Class{});
  }

  void generic_fill(FunctionBuilder& builder, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
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
};

// @TODO: does this really need to be a Component ?
class FunctionProcessor : public Component
{
public:
  PrototypeResolver prototype_;

public:
  using Component::Component;

public:

  void generic_fill(FunctionBuilder& builder, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    prototype_.generic_fill(builder, fundecl, scp);

    if (fundecl->deleteKeyword.isValid())
      builder.setDeleted();
    else if (fundecl->defaultKeyword.isValid())
      builder.setDefaulted();

    if (fundecl->explicitKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->explicitKeyword };

      if (!fundecl->is<ast::ConstructorDecl>())
        throw CompilationFailure{ CompilerError::InvalidUseOfExplicitKeyword };

      builder.setExplicit();
    }
    else if (fundecl->staticKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->staticKeyword };

      if (!scp.isClass())
        throw CompilationFailure{ CompilerError::InvalidUseOfStaticKeyword };

      /// TODO: is the following line needed ?
      builder.blueprint_.parent_ = Symbol{ scp.asClass() };

      builder.setStatic();
    }
    else if (fundecl->virtualKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->virtualKeyword };

      SymbolKind k = builder.blueprint_.name_.kind();

      if((k != SymbolKind::Destructor && k != SymbolKind::Function) || !builder.blueprint_.parent().isClass())
        throw CompilationFailure{ CompilerError::InvalidUseOfVirtualKeyword };

      builder.setVirtual();

      if (fundecl->virtualPure.isValid())
        builder.setPureVirtual();
    }

    if (fundecl->constQualifier.isValid())
    {
      TranslationTarget target{ this, fundecl->constQualifier };

      if (!scp.isClass() || fundecl->is<ast::ConstructorDecl>() || fundecl->is<ast::DestructorDecl>() || fundecl->staticKeyword.isValid())
        throw CompilationFailure{ CompilerError::InvalidUseOfConstKeyword };

      builder.setConst();
    }

    builder.setAccessibility(scp.accessibility());
  }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
