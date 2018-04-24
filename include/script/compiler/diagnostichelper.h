// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H
#define LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H

#include "script/diagnosticmessage.h"
#include "script/ast/node.h"

namespace script
{

namespace diagnostic
{

class DefaultDiagnosticHelper
{
public:

  inline static diagnostic::pos_t pos(const std::shared_ptr<ast::Node> & node)
  {
    const auto & p = node->pos();
    return diagnostic::pos_t{ p.line, p.col };
  }

  inline static std::string str(const std::shared_ptr<ast::Identifier> & node)
  {
    return node->getName();
  }
};

} // namespace diagnostic

} // namespace script

#endif // LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H
