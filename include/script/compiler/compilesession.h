// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SESSION_H
#define LIBSCRIPT_COMPILE_SESSION_H

#include "libscriptdefs.h"

#include "script/diagnosticmessage.h"
#include "script/function.h"
#include "script/lambda.h"
#include "script/script.h"

#include <vector>

namespace script
{

namespace ast
{
class Node;
} // namespace ast

namespace compiler
{

class Component;
class Compiler;

class CompileSession
{
public:
  enum class State {
    ProcessingDeclarations,
    CompilingFunctions,
    Finished,
  };

private:
  Compiler* mCompiler;
  State mState;

public:
  CompileSession(Compiler *c);
  CompileSession(Compiler *c, const Script & s);


  inline Compiler* compiler() const { return mCompiler; }
  Engine* engine() const;
  inline State state() const { return mState; }
  inline void setState(State t) { mState = t; }

  SourceLocation location() const;

  struct {
    /// TODO : ideally we should store a list of generated lambdas and function types
    std::vector<Function> functions; /// generated function template instances
    std::vector<Class> classes; /// generated class template instances
    std::shared_ptr<program::Expression> expression;
    std::vector<Script> scripts;
  } generated;

  std::vector<diagnostic::DiagnosticMessage> messages;
  bool error;
  Script script; // the top-level script that is being compiled

  Script current_script; // the script that is being processed
  std::shared_ptr<ast::Node> current_node; // the node that is being processed
  parser::Token current_token; // the token (within the 'current_node') that is being processed

  diagnostic::MessageBuilder& messageBuilder();

  void log(const diagnostic::DiagnosticMessage & mssg);
  void log(const CompilationFailure& ex);

  void clear();
};

class TranslationTarget
{
private:
  const Component* m_component;
  Script m_current_script;
  std::shared_ptr<ast::Node> m_current_node;
  parser::Token m_current_token;

public:
  TranslationTarget(const Component* c, const Script& script, std::shared_ptr<ast::Node> node);
  TranslationTarget(const Component* c, std::shared_ptr<ast::Node> node);
  TranslationTarget(const Component* c, parser::Token tok);
  ~TranslationTarget();
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SESSION_H
