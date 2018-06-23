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
  CompileSession(Compiler *c);

  void start();
  void suspend();
  void resume();
  void abort();

  struct {
    std::vector<Class> classes;
    std::vector<Enum> enums;
    std::vector<Function> functions;
    std::vector<Lambda> lambdas;
    std::shared_ptr<program::Expression> expression;
  } generated;

  std::vector<diagnostic::Message> messages;
  bool error;

private:
  Compiler *mCompiler;
  bool mPauseFlag;
  bool mAbortFlag;
};


class CompilerComponent
{
public:
  CompilerComponent(Compiler *c, CompileSession *s);
  virtual ~CompilerComponent() = default;

  Engine * engine() const;
  Compiler * compiler() const;

  CompileSession * session() const;

protected:
  template<typename ComponentType, typename...Args>
  ComponentType * getComponent(const Args &... args)
  {
    return new ComponentType{ mCompiler, mSession, args... };
  }

  void log(const diagnostic::Message & mssg);
  void log(const CompilerException & exception);

  static std::string dstr(const std::shared_ptr<ast::Identifier> & id);

protected:
  Class build(const ClassBuilder & builder);
  Function build(const FunctionBuilder & builder);
  Enum build(const Enum &, const std::string & name);
  Lambda build(const Lambda &);

private:
  friend class CompileSession;
  Compiler* mCompiler;
  CompileSession *mSession;
};


class Compiler
{
public:
  Compiler(Engine *e);
  ~Compiler() = default;

  Engine * engine() const;
  CompileSession * session() const;

  std::shared_ptr<program::Expression> compile(const std::string & expr, Context context, Script script = Script{});
  bool compile(Script s);

protected:
  void wipe_out(CompileSession *s);
  void log(const CompilerException & exception);


private:
  Engine * mEngine;
  std::unique_ptr<CompileSession> mSession; /// TODO : do we need a stack of sessions ?
};

} // namespace compiler

diagnostic::MessageBuilder & operator<<(diagnostic::MessageBuilder & builder, const compiler::CompilerException & ex);

} // namespace script

#endif // LIBSCRIPT_COMPILER_H
