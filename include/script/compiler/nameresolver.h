// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAME_RESOLVER_H
#define LIBSCRIPT_NAME_RESOLVER_H

#include "script/namelookup.h"
#include "script/scope.h"
#include "script/ast/node.h"

namespace script
{

LIBSCRIPT_API NameLookup resolve_name(const std::shared_ptr<ast::Identifier>& name, const Scope& scp);

} // namespace script

#endif // LIBSCRIPT_NAME_RESOLVER_H
