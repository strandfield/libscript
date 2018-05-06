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

  Prototype process(const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
  {
    Prototype result;

    if (fundecl->is<ast::ConstructorDecl>())
      result.setReturnType(Type{ scp.asClass().id(), Type::ReferenceFlag | Type::ConstFlag });
    else if (fundecl->is<ast::DestructorDecl>())
      result.setReturnType(Type::Void);
    else
      result.setReturnType(type_.resolve(fundecl->returnType, scp));

    if (scp.type() == script::Scope::ClassScope && fundecl->staticKeyword == parser::Token::Invalid
      && fundecl->is<ast::ConstructorDecl>() == false)
    {
      Type thisType{ scp.asClass().id(), Type::ReferenceFlag | Type::ThisFlag };
      if (fundecl->constQualifier.isValid())
        thisType.setFlag(Type::ConstFlag);

      result.addArgument(thisType);
    }

    bool mustbe_defaulted = false;
    for (size_t i(0); i < fundecl->params.size(); ++i)
    {
      Type argtype = type_.resolve(fundecl->params.at(i).type, scp);
      if (fundecl->params.at(i).defaultValue != nullptr)
        argtype.setFlag(Type::OptionalFlag), mustbe_defaulted = true;
      else if (mustbe_defaulted)
        throw InvalidUseOfDefaultArgument{ dpos(fundecl->params.at(i).name) };
      result.addArgument(argtype);
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

    if (fundecl->virtualKeyword.isValid())
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
