// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_H
#define LIBSCRIPT_COMPILER_H

#include "libscriptdefs.h"

#include "script/context.h"
#include "script/diagnosticmessage.h"
#include "script/enum.h"
#include "script/lambda.h"
#include "script/operator.h"
#include "script/scope.h"
#include "script/script.h"
#include "script/template.h"

#include <queue>

namespace script
{

namespace parser
{
class Token;
} // namespace parser

namespace ast
{
class Expression;
class Identifier;
class Node;
} // namespace ast

namespace program
{
class Expression;
} // namespace program

namespace compiler
{

class Compiler;
class CompilerException;
class CompileSession;

class CompileSession
{
public:
  CompileSession(Engine *e);

  inline Engine* engine() const { return mEngine; }

  struct {
    /// TODO : store a list of generated scripts
    std::vector<ClosureType> lambdas;
    std::vector<Function> functions; /// generated function template instances
    std::vector<Class> classes; /// generated class template instances
    std::shared_ptr<program::Expression> expression;
    std::vector<Script> scripts;
  } generated;

  std::vector<diagnostic::Message> messages;
  bool error;

  void clear();

private:
  Engine *mEngine;
};


class Compiler
{
public:
  explicit Compiler(Engine *e);
  explicit Compiler(std::shared_ptr<CompileSession> s);
  ~Compiler() = default;

  inline Engine * engine() const { return session()->engine(); }
  inline const std::shared_ptr<CompileSession> & session() const { return mSession; }

  bool compile(Script s);

protected:
  ClosureType newLambda();

protected:
  void log(const diagnostic::Message & mssg);
  void log(const CompilerException & exception);

private:
  std::shared_ptr<CompileSession> mSession;
};

} // namespace compiler

diagnostic::MessageBuilder & operator<<(diagnostic::MessageBuilder & builder, const compiler::CompilerException & ex);

} // namespace script

#endif // LIBSCRIPT_COMPILER_H
