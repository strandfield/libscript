// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/compiler.h"

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/parser/parser.h"

#include "script/compiler/commandcompiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "script/private/class_p.h"
#include "script/private/scope_p.h"
#include "script/private/script_p.h"

namespace script
{

namespace compiler
{

CompileSession::CompileSession(Engine *e)
  : mEngine(e)
  , mPauseFlag(false)
  , mAbortFlag(false)
  , error(false)
{

}

void CompileSession::start()
{
  resume();
}

void CompileSession::suspend()
{
  mPauseFlag = true;
}

void CompileSession::resume()
{
  mPauseFlag = false; /// TODO : should not be necessary

}

void CompileSession::abort()
{
  mPauseFlag = true;
  mAbortFlag = true;
}

void CompileSession::clear()
{
  /// TODO !!
  this->generated.lambdas.clear();

  this->generated.expression = nullptr;
}


Compiler::Compiler(Engine *e)
  : mSession(std::make_shared<CompileSession>(e))
{
}

Compiler::Compiler(std::shared_ptr<CompileSession> s)
  : mSession(s)
{
}

bool Compiler::compile(Script s)
{
  mSession.get()->error = false;
  mSession.get()->messages.clear();

  parser::Parser parser{ s.source() };
  auto ast = parser.parse(s.source());

  assert(ast != nullptr);

  if (ast->hasErrors())
  {
    s.impl()->messages = ast->steal_messages();
    return false;
  }

  ScriptCompiler sc(session());

  CompileScriptTask task{ s, ast };
  try
  {
    sc.compile(task);
  }
  catch (const CompilerException & e)
  {
    log(e);
  }
  //catch (...)
  //{
  //  log(NotImplementedError{ "Unknown error" });
  //}

  if (session()->error)
  {
    session()->clear();
    s.impl()->messages = std::move(session()->messages);
    engine()->implementation()->destroy(Namespace{ s.impl() });
    return false;
  }

  return true;
}

ClosureType Compiler::newLambda()
{
  auto ret = engine()->implementation()->newLambda();
  session()->generated.lambdas.push_back(ret);
  return ret;
}

void Compiler::log(const diagnostic::Message & mssg)
{
  this->mSession->messages.push_back(mssg);
  if (mssg.severity() == diagnostic::Error)
    this->mSession->error = true;
}

void Compiler::log(const CompilerException & ex)
{
  auto mssg = diagnostic::error(engine());
  mssg << ex;
  mSession->messages.push_back(mssg.build());
  mSession->error = true;
}

} // namespace compiler

diagnostic::MessageBuilder & operator<<(diagnostic::MessageBuilder & builder, const compiler::CompilerException & ex)
{
  builder << diagnostic::Code{ ex.code() };
  if (ex.pos.line >= 0)
    builder << ex.pos;
  ex.print(builder);
  return builder;
}

} // namespace script
