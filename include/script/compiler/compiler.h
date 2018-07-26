// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_H
#define LIBSCRIPT_COMPILER_H

#include "libscriptdefs.h"

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

namespace compiler
{

class CompilerException;
class CompileSession;
class FunctionCompiler;
class ScriptCompiler;
class SessionManager;

class Compiler
{
public:
  explicit Compiler(Engine *e);
  ~Compiler();

  inline Engine* engine() const { return mEngine; }
  inline const std::shared_ptr<CompileSession> & session() const { return mSession; }

  bool hasActiveSession() const;

  bool compile(Script s);

  Class instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & targs);
  void instantiate(const std::shared_ptr<ast::FunctionDecl> & decl, Function & func, const Scope & scp);
  std::shared_ptr<program::Expression> compile(const std::string & cmmd, const Context & con, const Scope & scp);

private:
  ScriptCompiler * getScriptCompiler();
  FunctionCompiler * getFunctionCompiler();
  void processAllDeclarations();
  void finalizeSession();

private:
  friend class SessionManager;
  Engine* mEngine;
  std::shared_ptr<CompileSession> mSession;
  std::unique_ptr<ScriptCompiler> mScriptCompiler;
  std::unique_ptr<FunctionCompiler> mFunctionCompiler;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_H
