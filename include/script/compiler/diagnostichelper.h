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
  return node->getName();
}

inline std::string dstr(const AccessSpecifier & as)
{
  if (as == AccessSpecifier::Protected)
    return "protected";
  else if (as == AccessSpecifier::Private)
    return "private";
  return "public";
}


class DefaultDiagnosticHelper
{
public:

  inline static diagnostic::pos_t pos(const std::shared_ptr<ast::Node> & node)
  {
    return dpos(node);
  }

  inline static std::string str(const std::shared_ptr<ast::Identifier> & node)
  {
    return dstr(node);
  }
};

} // namespace diagnostic

using diagnostic::dpos;
using diagnostic::dstr;

} // namespace script

#endif // LIBSCRIPT_COMPILER_DIAGNOSTIC_HELPER_H
