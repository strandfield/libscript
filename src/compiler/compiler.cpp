// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/compiler.h"

#include "script/engine.h"
#include "../engine_p.h"

#include "script/parser/parser.h"

#include "script/compiler/commandcompiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "../class_p.h"
#include "../scope_p.h"
#include "../script_p.h"

namespace script
{

namespace compiler
{

CompileSession::CompileSession(Compiler *c)
  : mCompiler(c)
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


CompilerComponent::CompilerComponent(Compiler *c, CompileSession *s)
  : mCompiler(c)
  , mSession(s)
{

}

Engine * CompilerComponent::engine() const
{
  return this->mCompiler->engine();
}

Compiler * CompilerComponent::compiler() const
{
  return this->mCompiler;
}

CompileSession * CompilerComponent::session() const
{
  return this->mSession;
}

void CompilerComponent::log(const diagnostic::Message & mssg)
{
  this->mSession->messages.push_back(mssg);
  if (mssg.severity() == diagnostic::Error)
    this->mSession->error = true;
}

void CompilerComponent::log(const CompilerException & exception)
{
  auto mssg = diagnostic::error();
  if (exception.pos.line >= 0)
    mssg << exception.pos;
  mssg << exception.what();
  log(mssg.build());
}

diagnostic::pos_t CompilerComponent::dpos(const std::shared_ptr<ast::Node> & node)
{
  const auto & p = node->pos();
  return diagnostic::pos_t{ p.line, p.col };
}

diagnostic::pos_t CompilerComponent::dpos(const ast::Node & node)
{
  const auto & p = node.pos();
  return diagnostic::pos_t{ p.line, p.col };
}

diagnostic::pos_t CompilerComponent::dpos(const parser::Token & tok)
{
  return diagnostic::pos_t{ tok.line, tok.column };
}

std::string CompilerComponent::dstr(const Type & t) const
{ 
  return engine()->typeName(t); 
}

std::string CompilerComponent::dstr(const AccessSpecifier & as)
{
  if (as == AccessSpecifier::Protected)
    return "protected";
  else if (as == AccessSpecifier::Private)
    return "private";
  return "public";
}

std::string CompilerComponent::dstr(const std::shared_ptr<ast::Identifier> & id)
{
  return id->getName();
}

Class CompilerComponent::build(const ClassBuilder & builder)
{
  auto ret = engine()->newClass(builder);
  session()->generated.classes.push_back(ret);
  return ret;
}

Function CompilerComponent::build(const FunctionBuilder & builder)
{
  auto ret = engine()->newFunction(builder);
  session()->generated.functions.push_back(ret);
  return ret;
}

Enum CompilerComponent::build(const Enum &, const std::string & name)
{
  auto ret = engine()->newEnum(name);
  session()->generated.enums.push_back(ret);
  return ret;
}

Lambda CompilerComponent::build(const Lambda &)
{
  auto ret = engine()->implementation()->newLambda();
  session()->generated.lambdas.push_back(ret);
  return ret;
}



Compiler::Compiler(Engine *e)
  : mEngine(e)
  , mSession(nullptr)
{
  mSession = std::unique_ptr<CompileSession>(new CompileSession{ this });
}

Engine * Compiler::engine() const
{
  return mEngine;
}

CompileSession * Compiler::session() const
{
  return mSession.get();
}

std::shared_ptr<program::Expression> Compiler::compile(const std::string & expr, Context context, Script script)
{
  auto source = SourceFile::fromString(expr);
  parser::Parser parser{ source };
  auto ast = parser.parseExpression(source);

  if (ast->hasErrors())
    return nullptr;

  compiler::CommandCompiler c{ this, mSession.get() };
  if (!script.isNull())
    c.setScope(Scope{ script });
  
  return c.compile(ast->expression(), context);
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
    s.implementation()->messages = ast->steal_messages();
    return false;
  }

  ScriptCompiler sc(this, mSession.get());

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

  if (mSession.get()->error)
  {
    wipe_out(mSession.get());
    s.implementation()->messages = std::move(mSession->messages);
    return false;
  }

  return true;
}

void Compiler::wipe_out(CompileSession *s)
{
  for (Class c : s->generated.classes)
    engine()->implementation()->destroyClass(c);
  s->generated.classes.clear();

  for (Function f : s->generated.functions)
  {
    /// TODO : find scope and remove
  }
  s->generated.functions.clear();

  for (Enum e : s->generated.enums)
    engine()->implementation()->destroyEnum(e);
  s->generated.enums.clear();

  /// TODO !!
  s->generated.lambdas.clear();


  s->generated.expression = nullptr;
}

void Compiler::log(const CompilerException & exception)
{
  auto mssg = diagnostic::error();
  if (exception.pos.line >= 0)
    mssg << exception.pos;
  mssg << exception.what();

  auto m = mssg.build();
  mSession->messages.push_back(m);
  if (m.severity() == diagnostic::Error)
    mSession->error = true;
}


} // namespace compiler

} // namespace script
