// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_AST_P_H
#define LIBSCRIPT_AST_P_H

#include <vector>

#include "script/sourcefile.h"
#include "script/ast/node.h"

#include "script/diagnosticmessage.h"

namespace script
{

class Script;
class ScriptImpl;

namespace ast
{

class LIBSCRIPT_API AST
{
public:
  AST();
  AST(const Script & s);
  AST(const SourceFile & src);

  void add(const std::shared_ptr<Statement> & statement);

  std::string text(const parser::Token & tok);

  void log(const diagnostic::Message & mssg);

public:
  std::shared_ptr<ast::Node> root;
  std::weak_ptr<ScriptImpl> script;
  SourceFile source;
  bool hasErrors;
  std::vector<diagnostic::Message> messages;
};

} // namespace ast

} // namespace script

#endif // LIBSCRIPT_AST_P_H
