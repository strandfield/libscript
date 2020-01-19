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

inline std::string dstr(const std::shared_ptr<ast::Identifier> & node)
{
  if (node->is<ast::SimpleIdentifier>())
    return node->as<ast::SimpleIdentifier>().getName();
  else if (node->is<ast::TemplateIdentifier>())
    return node->as<ast::TemplateIdentifier>().getName();
  else
    return dstr(node->as<ast::ScopedIdentifier>().rhs);
}

} // namespace diagnostic

using diagnostic::dstr;

} // namespace script

#endif // LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H
