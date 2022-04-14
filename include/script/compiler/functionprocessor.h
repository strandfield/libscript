// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
#define LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H

#include "script/compiler/component.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"

#include "script/ast/node.h"

#include "script/function-blueprint.h"
#include "script/scope.h"

namespace script
{

namespace compiler
{

LIBSCRIPT_API void fill_prototype(FunctionBlueprint& blueprint, const std::shared_ptr<ast::FunctionDecl>& fundecl, const Scope& scp);

// @TODO: does this really need to be a Component ?
class FunctionProcessor : public Component
{
public:
  using Component::Component;

public:

  void generic_fill(FunctionBlueprint& blueprint, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    script::compiler::fill_prototype(blueprint, fundecl, scp);

    if (fundecl->deleteKeyword.isValid())
      blueprint.flags_.set(FunctionSpecifier::Delete);
    else if (fundecl->defaultKeyword.isValid())
      blueprint.flags_.set(FunctionSpecifier::Default);

    if (fundecl->explicitKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->explicitKeyword };

      if (!fundecl->is<ast::ConstructorDecl>())
        throw CompilationFailure{ CompilerError::InvalidUseOfExplicitKeyword };

      blueprint.flags_.set(FunctionSpecifier::Explicit);
    }
    else if (fundecl->staticKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->staticKeyword };

      if (!scp.isClass())
        throw CompilationFailure{ CompilerError::InvalidUseOfStaticKeyword };

      /// TODO: is the following line needed ?
      blueprint.parent_ = Symbol{ scp.asClass() };

      blueprint.setStatic();
    }
    else if (fundecl->virtualKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->virtualKeyword };

      SymbolKind k = blueprint.name_.kind();

      if((k != SymbolKind::Destructor && k != SymbolKind::Function) || !blueprint.parent().isClass())
        throw CompilationFailure{ CompilerError::InvalidUseOfVirtualKeyword };

      blueprint.flags_.set(FunctionSpecifier::Virtual);

      if (fundecl->virtualPure.isValid())
        blueprint.flags_.set(FunctionSpecifier::Pure);
    }

    if (fundecl->constQualifier.isValid())
    {
      TranslationTarget target{ this, fundecl->constQualifier };

      if (!scp.isClass() || fundecl->is<ast::ConstructorDecl>() || fundecl->is<ast::DestructorDecl>() || fundecl->staticKeyword.isValid())
        throw CompilationFailure{ CompilerError::InvalidUseOfConstKeyword };

      blueprint.prototype_.setParameter(0, Type::cref(blueprint.prototype_.at(0)));
    }

    blueprint.flags_.set(scp.accessibility());
  }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
