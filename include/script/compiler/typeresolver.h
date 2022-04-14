// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPE_RESOLVER_H
#define LIBSCRIPT_TYPE_RESOLVER_H

#include "script/scope.h"
#include "script/types.h"

#include "script/ast/node.h"

namespace script
{

namespace compiler
{

LIBSCRIPT_API Type resolve_type(const ast::QualifiedType& qt, const Scope& scp);

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_FUNCTION_H
