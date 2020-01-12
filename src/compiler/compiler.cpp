// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/compiler.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/component.h"

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/classtemplate.h"
#include "script/classtemplateinstancebuilder.h"

#include "script/parser/parser.h"

#include "script/compiler/commandcompiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "script/private/class_p.h"
#include "script/private/function_p.h"
#include "script/private/scope_p.h"
#include "script/private/script_p.h"
#include "script/private/template_p.h"

namespace script
{

namespace compiler
{

void SessionLogger::log(const diagnostic::DiagnosticMessage & mssg)
{
  session_->log(mssg);
}

void SessionLogger::log(const CompilerException & exception)
{
  session_->log(exception);
}

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

  c->mSession = std::make_shared<CompileSession>(c);
  mStartedSession = true;
}

SessionManager::SessionManager(Compiler *c, const Script & s)
  : mCompiler(c)
  , mStartedSession(false)
{
  if (c->hasActiveSession())
  {
    c->session()->generated.scripts.push_back(s);
    return;
  }

  c->mSession = std::make_shared<CompileSession>(c, s);
  mStartedSession = true;
}

SessionManager::~SessionManager()
{
  if (mStartedSession)
    mCompiler->session()->setState(CompileSession::State::Finished);
}


CompileSession::CompileSession(Compiler *c)
  : mCompiler(c)
  , mState(State::ProcessingDeclarations)
  , error(false)
  , mLogger(this)
{

}

CompileSession::CompileSession(Compiler *c, const Script & s)
  : mCompiler(c)
  , mState(State::ProcessingDeclarations)
  , script(s)
  , error(false)
  , mLogger(this)
{

}

Engine* CompileSession::engine() const
{
  return mCompiler->engine();
}

void CompileSession::clear()
{
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

  for (const auto & s : this->generated.scripts)
    engine()->destroy(s);

  this->generated.expression = nullptr;

  this->engine()->garbageCollect();
}


Component::Component(Compiler* c)
  : m_compiler{ c }
{

}

Component::~Component()
{

}

Engine* Component::engine() const
{
  return m_compiler->engine();
}

Compiler* Component::compiler() const
{
  return m_compiler;
}

const std::shared_ptr<CompileSession>& Component::session() const
{
  return m_compiler->session();
}

void Component::log(const diagnostic::DiagnosticMessage& mssg)
{
  session()->log(mssg);
}

void Component::log(const CompilerException& exception)
{
  session()->log(exception);
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
  return mSession != nullptr && mSession->state() != CompileSession::State::Finished;
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
      finalizeSession();
    }
    else
    {
      if (session()->state() == CompileSession::State::CompilingFunctions)
      {
        processAllDeclarations();
      }
    }

    s.impl()->loaded = true;
  }
  catch (const CompilerException & e)
  {
    session()->log(e);
  }
  catch (const NotImplemented & ex)
  {
    session()->log(diagnostic::error() << "NotImplemented: " << ex.message);
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
    else
    {
      /// TODO: should we throw ?
    }

    return false;
  }

  return true;
}

Class Compiler::instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & targs)
{
  SessionManager manager{ this };

  ScriptCompiler *sc = getScriptCompiler();

  try
  {
    Class result = sc->instantiate(ct, targs);

    if (manager.started_session())
    {
      finalizeSession();
    }
    else
    {
      session()->generated.classes.push_back(result);

      if (session()->state() == CompileSession::State::CompilingFunctions)
      {
        processAllDeclarations();
      }
    }

    return result;
  }
  catch (const CompilerException & ex)
  {
    session()->clear();

    auto mssg = diagnostic::error(engine());
    mssg << ex;

    throw TemplateInstantiationError{ mssg.build().to_string().data() };
  }
}

void Compiler::instantiate(const std::shared_ptr<ast::FunctionDecl> & decl, Function & func, const Scope & scp)
{
  assert(!func.instanceOf().isNull());

  SessionManager manager{ this };

  FunctionCompiler fc{ this };

  CompileFunctionTask task;
  task.declaration = decl;
  task.function = func;
  task.scope = scp;

  try
  {
    fc.compile(task);

    if (manager.started_session())
    {
      finalizeSession();
    }
    else
    {
      session()->generated.functions.push_back(func);

      if (session()->state() == CompileSession::State::CompilingFunctions)
      {
        processAllDeclarations();
      }
    }
  }
  catch (const CompilerException & ex)
  {
    session()->clear();

    auto mssg = diagnostic::error(engine());
    mssg << ex;

    throw TemplateInstantiationError{ mssg.build().to_string().data() };
  }
}

std::shared_ptr<program::Expression> Compiler::compile(const std::string & cmmd, const Context & con)
{
  SessionManager manager{ this };

  CommandCompiler cc{ this };
  auto result = cc.compile(cmmd, con);

  if (manager.started_session())
  {
    finalizeSession();
  }

  return result;
}

ScriptCompiler * Compiler::getScriptCompiler()
{
  if (mScriptCompiler == nullptr)
    mScriptCompiler = std::make_unique<ScriptCompiler>(this);

  return mScriptCompiler.get();
}

FunctionCompiler * Compiler::getFunctionCompiler()
{
  if (mFunctionCompiler == nullptr)
  {
    mFunctionCompiler = std::make_unique<FunctionCompiler>(this);
  }

  return mFunctionCompiler.get();
}

void Compiler::processAllDeclarations()
{
  ScriptCompiler *sc = getScriptCompiler();
  while (!sc->done())
    sc->processNext();
}

void Compiler::finalizeSession()
{
  if (mScriptCompiler == nullptr)
  {
    session()->setState(CompileSession::State::Finished);
    return;
  }
  
  session()->setState(CompileSession::State::CompilingFunctions);

  ScriptCompiler *sc = getScriptCompiler();

  while (session()->state() != CompileSession::State::Finished)
  {
    processAllDeclarations();

    FunctionCompiler *fc = getFunctionCompiler();
    auto & queue = sc->compileTasks();
    while (!queue.empty())
    {
      CompileFunctionTask task = queue.front();
      queue.pop();
      fc->compile(task);
    }

    if (sc->variableProcessor().empty())
    {
      session()->setState(CompileSession::State::Finished);
    }
    else
    {
      sc->variableProcessor().initializeVariables();
    }
  }

  for (Script s : session()->generated.scripts)
  {
    if (s != session()->script)
      s.run();
  }

  session()->setState(CompileSession::State::Finished);
}


void CompileSession::log(const diagnostic::DiagnosticMessage & mssg)
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

} // namespace compiler

} // namespace script
