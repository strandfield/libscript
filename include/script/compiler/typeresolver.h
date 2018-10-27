// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPE_RESOLVER_H
#define LIBSCRIPT_TYPE_RESOLVER_H

#include "script/engine.h"
#include "script/functiontype.h"
#include "script/namelookup.h"
#include "script/scope.h"
#include "script/types.h"

#include "script/ast/node.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/diagnostichelper.h"

namespace script
{

namespace compiler
{

class TypeResolverBase
{
public:
  static Type complete(Type t, const ast::QualifiedType & qt)
  {
    if (qt.isRef())
      t = t.withFlag(Type::ReferenceFlag);
    else if (qt.isRefRef())
      t = t.withFlag(Type::ForwardReferenceFlag);

    if (qt.constQualifier.isValid())
      t = t.withFlag(Type::ConstFlag);

    return t;
  }
};

template<typename NameResolver>
class TypeResolver : public TypeResolverBase
{
private:
  NameResolver name_;
public:
  TypeResolver() = default;
  TypeResolver(const TypeResolver &) = default;
  ~TypeResolver() = default;

  inline NameResolver & name_resolver() { return name_; }

  Type resolve(const ast::QualifiedType & qt, const Scope & scp)
  {
    if (qt.isFunctionType())
      return complete(handle_function(qt.functionType, scp), qt);

    NameLookup lookup = name_.resolve(qt.type, scp);
    if (lookup.resultType() != NameLookup::TypeName)
      throw InvalidTypeName{ dpos(qt.type), dstr(qt.type) };

    return complete(lookup.typeResult(), qt);
  }

  Type resolve(const ast::QualifiedType & qt)
  {
    if (qt.isFunctionType())
      return complete(handle_function(qt.functionType), qt);

    NameLookup lookup = name_.resolve(qt.type);
    if (lookup.resultType() != NameLookup::TypeName)
      throw InvalidTypeName{ dpos(qt.type), dstr(qt.type) };

    return complete(lookup.typeResult(), qt);
  }

protected:
  Type handle_function(const std::shared_ptr<ast::FunctionType> & ft, const Scope & scp)
  {
    Prototype proto;
    proto.setReturnType(resolve(ft->returnType, scp));

    for (const auto & p : ft->params)
      proto.addParameter(resolve(p, scp));

    return scp.engine()->getFunctionType(proto).type();
  }

  Type handle_function(const std::shared_ptr<ast::FunctionType> & ft)
  {
    Prototype proto;
    proto.setReturnType(resolve(qt.functionType->returnType));

    for (const auto & p : qt.functionType->params)
      proto.addParameter(resolve(p));

    return scp.engine()->getFunctionType(proto).type();
  }
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_FUNCTION_H
