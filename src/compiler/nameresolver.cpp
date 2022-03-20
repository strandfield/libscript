// Copyright (C) 2019-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/nameresolver.h"

namespace script
{

/*!
 * \fn NameLookup resolve_name(const std::shared_ptr<ast::Identifier>& name, const Scope& scp)
 * \brief resolves a name within a given scope
 */
NameLookup resolve_name(const std::shared_ptr<ast::Identifier>& name, const Scope& scp)
{
  return NameLookup::resolve(name, scp);
}

} // namespace script

