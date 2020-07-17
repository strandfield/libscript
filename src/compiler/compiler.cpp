// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/compiler.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/component.h"

#include "script/ast.h"
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

#include <exception>

namespace script
{

namespace compiler
{

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
{

}

CompileSession::CompileSession(Compiler *c, const Script & s)
  : mCompiler(c)
  , mState(State::ProcessingDeclarations)
  , script(s)
  , error(false)
{
  current_script = s;
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

SourceLocation Component::location() const
{
  return session()->location();
}

void Component::log(const diagnostic::DiagnosticMessage& mssg)
{
  session()->log(mssg);
}

Compiler::Compiler(Engine *e)
  : mEngine(e),
    mMessageBuilder(std::make_shared<diagnostic::MessageBuilder>(e))
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
  assert(manager.started_session());

  ScriptCompiler *sc = getScriptCompiler();

  try
  {
    sc->add(s);
    finalizeSession();
    s.impl()->loaded = true;
  }
  catch (CompilationFailure& ex)
  {
    ex.location = session()->location();
    session()->log(ex);
  }
  catch (const NotImplemented & ex)
  {
    session()->log(DiagnosticMessage{diagnostic::Severity::Error, ex.errorCode(), "NotImplemented: "+ ex.message });
  }

  if (session()->error)
  {
    session()->clear();
    s.impl()->messages = std::move(session()->messages);
    engine()->implementation()->destroy(Namespace{ s.impl() });

    return false;
  }

  return true;
}

void Compiler::addToSession(Script s)
{
  SessionManager manager{ this, s };
  assert(!manager.started_session());

  ScriptCompiler* sc = getScriptCompiler();

  try
  {
    sc->add(s);

    if (session()->state() == CompileSession::State::CompilingFunctions)
    {
      processAllDeclarations();
    }

    s.impl()->loaded = true;
  }
  catch (CompilationFailure & ex)
  {
    ex.location = session()->location();
    session()->log(ex);
  }
  catch (const NotImplemented & ex)
  {
    session()->log(DiagnosticMessage{ diagnostic::Severity::Error, ex.errorCode(), "NotImplemented: " + ex.message });
  }
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
  catch (CompilationFailure& ex)
  {
    ex.location = session()->location();
    session()->clear();

    DiagnosticMessage mssg = messageBuilder()->error(ex);

    throw TemplateInstantiationError{ TemplateInstantiationError::CompilationFailure, mssg.to_string() };
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
  catch (CompilationFailure& ex)
  {
    ex.location = session()->location();
    session()->clear();

    DiagnosticMessage mssg = messageBuilder()->error(ex);

    throw TemplateInstantiationError{ TemplateInstantiationError::CompilationFailure, mssg.to_string() };
  }
}

/*!
 * \fn std::shared_ptr<program::Expression> compile(const std::string & cmmd, const Context & con)
 * \brief Compiles an expression within a context
 * Throws CompilationFailure if the compilation fails
 */
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

SourceLocation CompileSession::location() const
{
  SourceLocation loc;

  loc.m_source = current_script.source();

  if (current_node)
  {
    loc.m_pos = loc.m_source.map(current_node->pos());

    if (current_token.isValid())
    {
      loc.m_pos = this->current_script.ast().impl()->position(current_token);
    }
  }

  return loc;
}

diagnostic::MessageBuilder& CompileSession::messageBuilder()
{
  return *compiler()->messageBuilder();
}

void CompileSession::log(const diagnostic::DiagnosticMessage & mssg)
{
  this->messages.push_back(mssg);
  if (mssg.severity() == diagnostic::Error)
    this->error = true;
}

void CompileSession::log(const CompilationFailure& ex)
{
  DiagnosticMessage mssg = messageBuilder().error(ex);
  this->messages.push_back(mssg);
  this->error = true;
}

TranslationTarget::TranslationTarget(const Component* c, const Script& script, std::shared_ptr<ast::Node> node)
  : m_component(c),
    m_current_script(c->session()->current_script),
    m_current_node(c->session()->current_node),
    m_current_token(c->session()->current_token)
{
  c->session()->current_script = script;
  c->session()->current_node = node;
}

TranslationTarget::TranslationTarget(const Component* c, std::shared_ptr<ast::Node> node)
  : m_component(c),
    m_current_script(c->session()->current_script),
    m_current_node(c->session()->current_node),
    m_current_token(c->session()->current_token)
{
  c->session()->current_node = node;
}

TranslationTarget::TranslationTarget(const Component* c, parser::Token tok)
  : m_component(c),
    m_current_script(c->session()->current_script),
    m_current_node(c->session()->current_node),
    m_current_token(c->session()->current_token)
{

}

TranslationTarget::~TranslationTarget()
{
  if (!std::uncaught_exception())
  {
    m_component->session()->current_script = m_current_script;
    m_component->session()->current_node = m_current_node;
    m_component->session()->current_token = m_current_token;
  }
}

} // namespace compiler

} // namespace script
