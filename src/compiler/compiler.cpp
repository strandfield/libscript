// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/compiler.h"
#include "script/compiler/compilercomponent.h"
#include "script/compiler/compilesession.h"

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/classtemplate.h"

#include "script/parser/parser.h"

#include "script/compiler/commandcompiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "script/private/class_p.h"
#include "script/private/scope_p.h"
#include "script/private/script_p.h"
#include "script/private/template_p.h"

namespace script
{

diagnostic::MessageBuilder & operator<<(diagnostic::MessageBuilder & builder, const compiler::CompilerException & ex)
{
  builder << diagnostic::Code{ ex.code() };
  if (ex.pos.line >= 0)
    builder << ex.pos;
  ex.print(builder);
  return builder;
}

namespace compiler
{

class SessionManager
{
private:
  Compiler* mCompiler;
  bool mStartedSession;

public:
  explicit SessionManager(Compiler *c);
  SessionManager(Compiler *c, const Script &s);
  ~SessionManager();

  inline bool started_session() const { return mStartedSession; }
};

SessionManager::SessionManager(Compiler *c)
  : mCompiler(c)
  , mStartedSession(false)
{
  if (c->hasActiveSession())
    return;

  c->mSession = std::make_shared<CompileSession>(c->engine());
  mStartedSession = true;
}

SessionManager::SessionManager(Compiler *c, const Script & s)
  : mCompiler(c)
  , mStartedSession(false)
{
  if (c->hasActiveSession())
    return;

  c->mSession = std::make_shared<CompileSession>(s);
  mStartedSession = true;
}

SessionManager::~SessionManager()
{
  if (mStartedSession)
    mCompiler->session()->set_active(false);
}



CompileSession::CompileSession(Engine *e)
  : mEngine(e)
  , mIsActive(true)
  , error(false)
{

}

CompileSession::CompileSession(const Script & s)
  : mEngine(s.engine())
  , mIsActive(true)
  , script(s)
  , error(false)
{

}

void CompileSession::clear()
{
  /// TODO !!
  for (const auto & f : this->generated.functions)
  {
    if (!f.isTemplateInstance())
      continue;

    FunctionTemplate ft = f.instanceOf();
    auto & instances = ft.impl()->instances;
    instances.erase(f.arguments());
  }
  this->generated.functions.clear();

  for (const auto & c : this->generated.classes)
  {
    if (!c.isTemplateInstance())
      continue;

    ClassTemplate ct = c.instanceOf();
    auto & instances = ct.impl()->instances;
    instances.erase(c.arguments());
  }
  this->generated.classes.clear();

  for (const auto & l : this->generated.lambdas)
    engine()->implementation()->destroy(l);
  this->generated.lambdas.clear();

  for (const auto & s : this->generated.scripts)
    engine()->destroy(s);

  this->generated.expression = nullptr;

  this->engine()->garbageCollect();
}


Compiler::Compiler(Engine *e)
  : mEngine(e)
{

}

Compiler::~Compiler()
{

}

bool Compiler::hasActiveSession() const
{
  return mSession != nullptr && mSession->is_active();
}

bool Compiler::compile(Script s)
{
  SessionManager manager{ this, s };

  ScriptCompiler *sc = getScriptCompiler();

  try
  {
    sc->add(s);

    if (manager.started_session())
    {
      while (!sc->done())
        sc->processNext();
    }
  }
  catch (const CompilerException & e)
  {
    session()->log(e);
  }
  //catch (...)
  //{
  //  log(NotImplementedError{ "Unknown error" });
  //}

  if (session()->error)
  {
    if (manager.started_session())
    {
      session()->clear();
      s.impl()->messages = std::move(session()->messages);
      engine()->implementation()->destroy(Namespace{ s.impl() });
    }
    return false;
  }

  return true;
}

Class Compiler::instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & targs)
{
  SessionManager manager{ this };

  ScriptCompiler *sc = getScriptCompiler();

  Class result = sc->instantiate2(ct, targs);
  if (manager.started_session())
  {
    while (!sc->done())
      sc->processNext();
  }

  return result;
}

void Compiler::instantiate(const std::shared_ptr<ast::FunctionDecl> & decl, Function & func, const Scope & scp)
{
  SessionManager manager{ this };

  FunctionCompiler fc{ this };

  CompileFunctionTask task;
  task.declaration = decl;
  task.function = func;
  task.scope = scp;

  fc.compile(task);

  if (manager.started_session())
  {
    if (mScriptCompiler != nullptr)
    {
      while (!mScriptCompiler->done())
        mScriptCompiler->processNext();
    }

    /// TODO: run imported scripts
  }
}

std::shared_ptr<program::Expression> Compiler::compile(const std::string & cmmd, const Context & con, const Scope & scp)
{
  SessionManager manager{ this };

  CommandCompiler cc{ this };
  auto result = cc.compile(cmmd, con, scp);

  if (manager.started_session())
  {
    if (mScriptCompiler != nullptr)
    {
      while (!mScriptCompiler->done())
        mScriptCompiler->processNext();
    }

    /// TODO: run imported scripts
  }

  return result;
}

ScriptCompiler * Compiler::getScriptCompiler()
{
  if (mScriptCompiler == nullptr)
    mScriptCompiler = std::make_unique<ScriptCompiler>(this);

  return mScriptCompiler.get();
}

ClosureType CompileSession::newLambda()
{
  auto ret = engine()->implementation()->newLambda();
  this->generated.lambdas.push_back(ret);
  return ret;
}

CompilerComponent::CompilerComponent(Compiler *c)
  : mCompiler(c)
{

}

Engine* CompilerComponent::engine() const
{
  return session()->engine();
}

std::shared_ptr<CompileSession> CompilerComponent::session() const
{
  return compiler()->session();
}

void CompileSession::log(const diagnostic::Message & mssg)
{
  this->messages.push_back(mssg);
  if (mssg.severity() == diagnostic::Error)
    this->error = true;
}

void CompileSession::log(const CompilerException & ex)
{
  auto mssg = diagnostic::error(engine());
  mssg << ex;
  this->messages.push_back(mssg.build());
  this->error = true;
}

void CompilerComponent::log(const diagnostic::Message & mssg)
{
  session()->log(mssg);
}

void CompilerComponent::log(const CompilerException & ex)
{
  session()->log(ex);
}

} // namespace compiler

} // namespace script
