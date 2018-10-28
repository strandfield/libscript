// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
#define LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H

#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/class.h"

#include <vector>

namespace script
{

namespace compiler
{

template<typename TypeResolver>
class BasicPrototypeResolver
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


template<typename PrototypeResolver>
class FunctionProcessor
{
public:
  PrototypeResolver prototype_;

public:
  FunctionProcessor() = default;

  template<typename Builder>
  void generic_fill(Builder & builder, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    /// TODO: maybe all the exceptions can be removed here,
    // we could just catch the execptions thrown by the builder and rewrite them.

    prototype_.generic_fill(builder, fundecl, scp);

    if (fundecl->deleteKeyword.isValid())
      builder.setDeleted();
    else if (fundecl->defaultKeyword.isValid())
      builder.setDefaulted();

    if (fundecl->explicitKeyword.isValid())
    {
      if (!fundecl->is<ast::ConstructorDecl>())
        throw NotImplementedError{ dpos(fundecl->explicitKeyword), "Invalid use of explicit keyword" };

      builder.setExplicit();
    }
    else if (fundecl->staticKeyword.isValid())
    {
      if (!scp.isClass())
        throw InvalidUseOfStaticKeyword{ dpos(fundecl->staticKeyword) };

      /// TODO: is the following line needed ?
      builder.symbol = Symbol{ scp.asClass() };

      builder.setStatic();
    }
    else if (fundecl->virtualKeyword.isValid())
    {
      try
      {
        builder.setVirtual();
        if (fundecl->virtualPure.isValid())
          builder.setPureVirtual();
      }
      catch (...)
      {
        throw InvalidUseOfVirtualKeyword{ dpos(fundecl->virtualKeyword) };
      }
    }

    if (fundecl->constQualifier.isValid())
    {
      if (!scp.isClass() || fundecl->is<ast::ConstructorDecl>() || fundecl->is<ast::DestructorDecl>() || fundecl->staticKeyword.isValid())
        throw InvalidUseOfConstKeyword{ dpos(fundecl->constQualifier) };
      builder.setConst();
    }

    builder.setAccessibility(scp.accessibility());
  }

};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
