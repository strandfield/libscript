// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_AST_H
#define LIBSCRIPT_AST_H

#include <vector>

#include "script/sourcefile.h"
#include "script/diagnosticmessage.h"

namespace script
{

namespace ast
{
class AST;
class Declaration;
class Expression;
class Node;
class Statement;
} // namespace ast

class Script;

class LIBSCRIPT_API Ast
{
public:
  Ast() = default;
  Ast(const Ast &) = default;
  ~Ast() = default;

  inline bool isNull() const { return d == nullptr; }

  SourceFile source() const;

  const std::shared_ptr<ast::Node> & root() const;

  bool hasErrors() const;
  const std::vector<diagnostic::Message> & messages() const;

  bool isScript() const;
  Script script() const;
  const std::vector<std::shared_ptr<ast::Statement>> & statements() const;
  const std::vector<std::shared_ptr<ast::Declaration>> & declarations() const;

  bool isExpression() const;
  std::shared_ptr<ast::Expression> expression() const;

  inline const std::shared_ptr<ast::AST> & impl() const { return d; }

  Ast & operator=(const Ast &) = default;

public:
  std::shared_ptr<ast::AST> d;
};

} // namespace script

#endif // LIBSCRIPT_AST_H
