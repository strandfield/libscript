// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
#define LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H

#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

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

  Prototype process(const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    Prototype result;

    const Class class_scope = getClass(scp);

    if (fundecl->is<ast::ConstructorDecl>())
      result.setReturnType(Type{ class_scope.id(), Type::ReferenceFlag | Type::ConstFlag });
    else if (fundecl->is<ast::DestructorDecl>())
      result.setReturnType(Type::Void);
    else
      result.setReturnType(type_.resolve(fundecl->returnType, scp));

    if (!class_scope.isNull() && fundecl->staticKeyword == parser::Token::Invalid
      && fundecl->is<ast::ConstructorDecl>() == false)
    {
      Type thisType{ class_scope.id(), Type::ReferenceFlag | Type::ThisFlag };
      if (fundecl->constQualifier.isValid())
        thisType.setFlag(Type::ConstFlag);

      result.addParameter(thisType);
    }

    for (size_t i(0); i < fundecl->params.size(); ++i)
    {
      Type argtype = type_.resolve(fundecl->params.at(i).type, scp);
      result.addParameter(argtype);
    }

    return result;
  }

};


template<typename PrototypeResolver>
class FunctionProcessor
{
public:
  PrototypeResolver prototype_;

public:
  FunctionProcessor() = default;

  void fill(FunctionBuilder & builder, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    builder.proto = prototype_.process(fundecl, scp);

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

      builder.symbol = Symbol{ scp.asClass() };
      builder.setStatic();
    }
    else if (fundecl->virtualKeyword.isValid())
    {
      if (!scp.isClass() || (builder.kind != Function::StandardFunction && builder.kind != Function::Destructor))
        throw InvalidUseOfVirtualKeyword{ dpos(fundecl->virtualKeyword) };

      builder.setVirtual();
      if (fundecl->virtualPure.isValid())
        builder.setPureVirtual();
    }

    builder.setAccessibility(scp.accessibility());
  }

};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_FUNCTION_PROCESSOR_H
