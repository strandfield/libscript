// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPE_RESOLVER_H
#define LIBSCRIPT_TYPE_RESOLVER_H

#include "script/engine.h"
#include "script/functiontype.h"
#include "script/namelookup.h"
#include "script/scope.h"
#include "script/typesystem.h"

#include "script/ast/node.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/diagnostichelper.h"
#include "script/compiler/nameresolver.h"

namespace script
{

namespace compiler
{

class LIBSCRIPT_API TypeResolver
{
private:
  NameResolver name_;
public:
  TypeResolver() = default;
  TypeResolver(const TypeResolver&) = default;
  ~TypeResolver() = default;

  inline NameResolver& name_resolver() { return name_; }

  Type resolve(const ast::QualifiedType& qt, const Scope& scp);

  static Type complete(Type t, const ast::QualifiedType& qt);

protected:
  Type handle_function(const std::shared_ptr<ast::FunctionType>& ft, const Scope& scp);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_FUNCTION_H
