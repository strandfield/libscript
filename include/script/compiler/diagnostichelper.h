// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H
#define LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H

#include "script/accessspecifier.h"
#include "script/diagnosticmessage.h"
#include "script/ast/node.h"

namespace script
{

namespace diagnostic
{

inline diagnostic::pos_t dpos(const std::shared_ptr<ast::Node> & node)
{
  const auto & p = node->pos();
  return diagnostic::pos_t{ p.line, p.col };
}

inline diagnostic::pos_t dpos(const ast::Node & node)
{
  const auto & p = node.pos();
  return diagnostic::pos_t{ p.line, p.col };
}

inline diagnostic::pos_t dpos(const parser::Token & tok)
{
  return diagnostic::pos_t{ tok.line, tok.column };
}

inline std::string dstr(const std::shared_ptr<ast::Identifier> & node)
{
  if (!node->is<ast::ScopedIdentifier>())
    return node->getName();
  else
    return dstr(node->as<ast::ScopedIdentifier>().rhs);
}

} // namespace diagnostic

using diagnostic::dpos;
using diagnostic::dstr;

} // namespace script

#endif // LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H
