// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAME_RESOLVER_H
#define LIBSCRIPT_NAME_RESOLVER_H

#include "script/namelookup.h"
#include "script/scope.h"

#include "script/ast/node.h"

namespace script
{

namespace compiler
{

class BasicNameResolver
{
public:
  BasicNameResolver() = default;
  BasicNameResolver(const BasicNameResolver &) = default;
  ~BasicNameResolver() = default;

  inline static NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scp)
  {
    return NameLookup::resolve(name, scp);
  }

  BasicNameResolver & operator=(const BasicNameResolver &) = default;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_NAME_RESOLVER_H
