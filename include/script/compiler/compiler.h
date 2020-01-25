// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_H
#define LIBSCRIPT_COMPILER_H

#include "libscriptdefs.h"

#include <memory>
#include <vector>

namespace script
{

class Class;
class ClassTemplate;
class Context;
class Engine;
class Function;
class Scope;
class Script;
class TemplateArgument;

namespace ast
{
class FunctionDecl;
}

namespace program
{
class Expression;
} // namespace program

namespace diagnostic
{
class MessageBuilder;
} // namespace diagnostic

namespace compiler
{

class Compiler;
class CompileSession;
class FunctionCompiler;
class ScriptCompiler;
class SessionManager;

class LIBSCRIPT_API SessionManager
{
private:
  Compiler* mCompiler;
  bool mStartedSession;

public:
  explicit SessionManager(Compiler* c);
  SessionManager(Compiler* c, const Script& s);
  ~SessionManager();

  inline bool started_session() const { return mStartedSession; }
};

class Compiler
{
public:
  explicit Compiler(Engine *e);
  ~Compiler();

  Engine* engine() const { return mEngine; }
  const std::shared_ptr<CompileSession> & session() const { return mSession; }
  const std::shared_ptr<diagnostic::MessageBuilder>& messageBuilder() const { return mMessageBuilder; }

  bool hasActiveSession() const;

  bool compile(Script s);

  void addToSession(Script s);

  Class instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & targs);
  void instantiate(const std::shared_ptr<ast::FunctionDecl> & decl, Function & func, const Scope & scp);
  std::shared_ptr<program::Expression> compile(const std::string & cmmd, const Context & con);

private:
  ScriptCompiler * getScriptCompiler();
  FunctionCompiler * getFunctionCompiler();
  void processAllDeclarations();
  void finalizeSession();

private:
  friend class SessionManager;
  Engine* mEngine;
  std::shared_ptr<diagnostic::MessageBuilder> mMessageBuilder;
  std::shared_ptr<CompileSession> mSession;
  std::unique_ptr<ScriptCompiler> mScriptCompiler;
  std::unique_ptr<FunctionCompiler> mFunctionCompiler;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_H
