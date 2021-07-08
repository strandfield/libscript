// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
#define LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H

#include "script/compiler/component.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/class.h"
#include "script/symbol.h"

#include <vector>

namespace script
{

namespace compiler
{

class PrototypeResolver
{
public:
  TypeResolver type_;

  inline Class getClass(const Scope & scp)
  {
    return scp.isClass() ? scp.asClass() : (scp.type() == Scope::TemplateArgumentScope ? getClass(scp.parent()) : Class{});
  }

  template<typename Builder>
  void generic_fill(Builder & builder, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    const Class class_scope = getClass(scp);

    if (!fundecl->is<ast::ConstructorDecl>() && !fundecl->is<ast::DestructorDecl>())
      builder.returns(type_.resolve(fundecl->returnType, scp));

    for (size_t i(0); i < fundecl->params.size(); ++i)
    {
      Type argtype = type_.resolve(fundecl->params.at(i).type, scp);
      builder.params(argtype);
    }
  }
};

class FunctionProcessor : public Component
{
public:
  PrototypeResolver prototype_;

public:
  using Component::Component;

protected:

  struct selector { };

  template<typename Builder>
  void set_explicit(Builder& builder, ...)
  {
    // @TODO: use CompilationFailure exception
    throw std::runtime_error{ "Builder does not support 'explicit' specifier" };
  }

  template<typename Builder>
  void set_explicit(Builder& builder, decltype(std::declval<Builder>().setExplicit(), selector()))
  {
    builder.setExplicit();
  }

  template<typename Builder>
  void set_virtual(Builder& builder, ...)
  {
    // @TODO: use CompilationFailure exception
    throw std::runtime_error{ "Builder does not support 'virtual' specifier" };
  }

  template<typename Builder>
  void set_virtual(Builder& builder, decltype(std::declval<Builder>().setVirtual(), selector()))
  {
    builder.setVirtual();
  }

  template<typename Builder>
  void set_virtual_pure(Builder& builder, ...)
  {
    // @TODO: use CompilationFailure exception
    throw std::runtime_error{ "Builder does not support 'virtual' specifier" };
  }

  template<typename Builder>
  void set_virtual_pure(Builder& builder, decltype(std::declval<Builder>().setPureVirtual(), selector()))
  {
    builder.setPureVirtual();
  }

  template<typename Builder>
  void set_const(Builder& builder, ...)
  {
    // @TODO: use CompilationFailure exception
    throw std::runtime_error{ "Builder does not support 'const' specifier" };
  }

  template<typename Builder>
  void set_const(Builder& builder, decltype(std::declval<Builder>().setConst(), selector()))
  {
    builder.setConst();
  }

  template<typename Builder>
  void set_default(Builder& builder, ...)
  {
    // @TODO: use CompilationFailure exception
    throw std::runtime_error{ "Builder does not support 'default' specifier" };
  }

  template<typename Builder>
  void set_default(Builder& builder, decltype(std::declval<Builder>().setDefaulted(), selector()))
  {
    builder.setDefaulted();
  }

  template<typename Builder>
  void set_delete(Builder& builder, ...)
  {
    // @TODO: use CompilationFailure exception
    throw std::runtime_error{ "Builder does not support 'default' specifier" };
  }

  template<typename Builder>
  void set_delete(Builder& builder, decltype(std::declval<Builder>().setDeleted(), selector()))
  {
    builder.setDeleted();
  }

  template<typename Builder>
  void set_static(Builder& builder, ...)
  {
    // @TODO: use CompilationFailure exception
    throw std::runtime_error{ "Builder does not support 'static' specifier" };
  }

  template<typename Builder>
  void set_static(Builder& builder, decltype(std::declval<Builder>().setStatic(), selector()))
  {
    builder.setStatic();
  }

public:

  template<typename Builder>
  void generic_fill(Builder & builder, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    /// TODO: maybe all the exceptions can be removed here,
    // we could just catch the execptions thrown by the builder and rewrite them.

    prototype_.generic_fill(builder, fundecl, scp);

    if (fundecl->deleteKeyword.isValid())
      set_delete(builder, selector());
    else if (fundecl->defaultKeyword.isValid())
      set_default(builder, selector());

    if (fundecl->explicitKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->explicitKeyword };

      if (!fundecl->is<ast::ConstructorDecl>())
        throw CompilationFailure{ CompilerError::InvalidUseOfExplicitKeyword };

      set_explicit(builder, selector());
    }
    else if (fundecl->staticKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->staticKeyword };

      if (!scp.isClass())
        throw CompilationFailure{ CompilerError::InvalidUseOfStaticKeyword };

      /// TODO: is the following line needed ?
      builder.symbol = Symbol{ scp.asClass() };

      set_static(builder, selector());
    }
    else if (fundecl->virtualKeyword.isValid())
    {
      TranslationTarget target{ this, fundecl->virtualKeyword };

      try
      {
        set_virtual(builder, selector());

        if (fundecl->virtualPure.isValid())
          set_virtual_pure(builder, selector());
      }
      catch (...)
      {
        throw CompilationFailure{ CompilerError::InvalidUseOfVirtualKeyword };
      }
    }

    if (fundecl->constQualifier.isValid())
    {
      TranslationTarget target{ this, fundecl->constQualifier };

      if (!scp.isClass() || fundecl->is<ast::ConstructorDecl>() || fundecl->is<ast::DestructorDecl>() || fundecl->staticKeyword.isValid())
        throw CompilationFailure{ CompilerError::InvalidUseOfConstKeyword };

      set_const(builder, selector());
    }

    builder.setAccessibility(scp.accessibility());
  }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
