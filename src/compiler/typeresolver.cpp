// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/typeresolver.h"

namespace script
{

namespace compiler
{

Type TypeResolver::resolve(const ast::QualifiedType& qt, const Scope& scp)
{
  if (qt.isFunctionType())
    return complete(handle_function(qt.functionType, scp), qt);

  NameLookup lookup = name_.resolve(qt.type, scp);
  if (lookup.resultType() != NameLookup::TypeName)
    throw InvalidTypeName{ dpos(qt.type), dstr(qt.type) };

  return complete(lookup.typeResult(), qt);
}

Type TypeResolver::complete(Type t, const ast::QualifiedType & qt)
{
  if (qt.isRef())
    t = t.withFlag(Type::ReferenceFlag);
  else if (qt.isRefRef())
    t = t.withFlag(Type::ForwardReferenceFlag);

  if (qt.constQualifier.isValid())
    t = t.withFlag(Type::ConstFlag);

  return t;
}

Type TypeResolver::handle_function(const std::shared_ptr<ast::FunctionType> & ft, const Scope & scp)
{
  DynamicPrototype proto;
  proto.setReturnType(resolve(ft->returnType, scp));

  for (const auto& p : ft->params)
    proto.push(resolve(p, scp));

  return scp.engine()->typeSystem()->getFunctionType(proto).type();
}

} // namespace compiler

} // namespace script

