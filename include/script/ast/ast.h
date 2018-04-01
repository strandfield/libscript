// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_AST_H
#define LIBSCRIPT_AST_H

#include <vector>

#include "script/sourcefile.h"
#include "script/ast/node.h"

#include "script/diagnosticmessage.h"

namespace script
{

namespace ast
{

class LIBSCRIPT_API AST
{
public:
  AST();
  AST(const SourceFile & src);

  const std::vector<std::shared_ptr<Statement>> & statements() const;
  const std::vector<std::shared_ptr<Declaration>> & declarations() const;

  void add(const std::shared_ptr<Statement> & statement);

  bool isSingleExpression() const;
  void setExpression(const std::shared_ptr<Expression> & e);
  std::shared_ptr<Expression> expression() const;

  std::string text(const parser::Token & tok);

  bool hasErrors() const;
  void setErrorFlag(bool on = true);

  void log(const diagnostic::Message & mssg);
  const std::vector<diagnostic::Message> & messages() const;
  inline std::vector<diagnostic::Message> && steal_messages() { return std::move(mMessages); }

protected:
  SourceFile mSource;
  std::vector<std::shared_ptr<Statement>> mStatements;
  std::vector<std::shared_ptr<Declaration>> mDeclarations;
  std::shared_ptr<Expression> mExpression;
  bool mErrors;
  std::vector<diagnostic::Message> mMessages;
};

} // namespace ast

} // namespace script

#endif // LIBSCRIPT_AST_H
