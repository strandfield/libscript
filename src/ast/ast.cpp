// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/ast/ast.h"

namespace script
{

namespace ast
{

AST::AST()
  : mErrors(false)
{

}

AST::AST(const SourceFile & src)
  : mErrors(false)
{
  mSource = src;
}

const std::vector<std::shared_ptr<Statement>> & AST::statements() const
{
  return mScriptNode->statements;
}

const std::vector<std::shared_ptr<Declaration>> & AST::declarations() const
{
  return mScriptNode->declarations;
}

void AST::add(const std::shared_ptr<Statement> & statement)
{
  mScriptNode->statements.push_back(statement);
  if (statement->is<Declaration>())
    mScriptNode->declarations.push_back(std::dynamic_pointer_cast<Declaration>(statement));
}

bool AST::isSingleExpression() const
{
  return mExpression != nullptr;
}

void AST::setExpression(const std::shared_ptr<Expression> & e)
{
  mExpression = e;
}

std::shared_ptr<Expression> AST::expression() const
{
  return mExpression;
}

std::string AST::text(const parser::Token & tok)
{
  return std::string(mSource.data() + tok.pos, tok.length);
}

bool AST::hasErrors() const
{
  return mErrors;
}

void AST::setErrorFlag(bool on)
{
  mErrors = on;
}

void AST::log(const diagnostic::Message & mssg)
{
  mMessages.push_back(mssg);
}

const std::vector<diagnostic::Message> & AST::messages() const
{
  return mMessages;
}


} // namespace ast

} // namespace script
