// Copyright (C) 2019-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/typeresolver.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/diagnostichelper.h"
#include "script/compiler/nameresolver.h"

#include "script/engine.h"
#include "script/functiontype.h"
#include "script/prototypes.h"
#include "script/typesystem.h"

namespace script
{

namespace compiler
{

static Type complete(Type t, const ast::QualifiedType& qt)
{
  if (qt.isRef())
    t = t.withFlag(Type::ReferenceFlag);
  else if (qt.isRefRef())
    t = t.withFlag(Type::ForwardReferenceFlag);

  if (qt.constQualifier.isValid())
    t = t.withFlag(Type::ConstFlag);

  return t;
}

static Type handle_function(const std::shared_ptr<ast::FunctionType>& ft, const Scope& scp)
{
  DynamicPrototype proto;
  proto.setReturnType(resolve_type(ft->returnType, scp));

  for (const auto& p : ft->params)
    proto.push(resolve_type(p, scp));

  return scp.engine()->typeSystem()->getFunctionType(proto).type();
}

/*!
 * \fn Type resolve_type(const ast::QualifiedType& qt, const Scope& scp)
 * \brief resolves a type within a scope
 * \param the qualified type
 * \param the resolution scope
 */
Type resolve_type(const ast::QualifiedType& qt, const Scope& scp)
{
  if (qt.isFunctionType())
    return complete(handle_function(qt.functionType, scp), qt);

  NameLookup lookup = script::resolve_name(qt.type, scp);

  // @TODO: throw another exception, e.g. TypeResolutionFailure
  if (lookup.resultType() != NameLookup::TypeName)
    throw CompilationFailure{ CompilerError::InvalidTypeName, errors::InvalidName{dstr(qt.type)} };

  return complete(lookup.typeResult(), qt);
}

} // namespace compiler

} // namespace script

