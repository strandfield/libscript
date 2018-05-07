// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SCRIPT_H
#define LIBSCRIPT_COMPILE_SCRIPT_H

#include "script/compiler/compiler.h"

#include "script/compiler/compilefunctiontask.h"
#include "script/compiler/expressioncompiler.h"
#include "script/compiler/functionprocessor.h"
#include "script/compiler/importprocessor.h"
#include "script/compiler/scopestatementprocessor.h"
#include "script/compiler/typeresolver.h"
#include "script/compiler/variableprocessor.h"

#include "script/types.h"
#include "script/engine.h"

#include "script/ast/forwards.h"

#include <vector>

namespace script
{

class NameLookup;
class Template;
class TemplateArgument;

namespace compiler
{

class Compiler;

struct StaticVariable
{
  StaticVariable() { }
  StaticVariable(const Value & var, const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp) :
    variable(var), declaration(decl), scope(scp) { }

  Value variable;
  std::shared_ptr<ast::VariableDecl> declaration;
  Scope scope;
};


struct ScopedDeclaration
{
  ScopedDeclaration() { }
  ScopedDeclaration(const script::Scope & scp, const std::shared_ptr<ast::Declaration> & decl) : scope(scp), declaration(decl) { }

  Scope scope;
  std::shared_ptr<ast::Declaration> declaration;
};

struct IncompleteFunction : public ScopedDeclaration
{
  Function function;
  IncompleteFunction(const Scope & scp, const std::shared_ptr<ast::Declaration> & decl, const Function & func) 
    : ScopedDeclaration(scp, decl), function(func) { }
};

struct CompileScriptTask
{
  Script script;
  std::shared_ptr<ast::AST> ast;
};

class ScriptCompiler;

class ScriptCompilerNameResolver
{
public:
  ScriptCompiler* compiler;
public:
  ScriptCompilerNameResolver() = default;
  ScriptCompilerNameResolver(const ScriptCompilerNameResolver &) = default;

  inline Engine* engine() const;

  inline NameLookup resolve(const std::shared_ptr<ast::Identifier> & name);

  inline NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scp)
  {
    return NameLookup::resolve(name, scp);
  }
};

class ScriptCompilerModuleLoader
{
public:
  ScriptCompiler *compiler_;
  Engine* engine() const;

  Script load(const SourceFile &src);
};

class ScriptCompiler : public CompilerComponent
{
public:
  ScriptCompiler(Compiler *c, CompileSession *s);

  void compile(const CompileScriptTask & task);

  Class compileClassTemplate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args, const std::shared_ptr<ast::ClassDecl> & class_decl);

  inline Script script() const { return mCurrentScript; }
  inline const Scope & currentScope() const { return mCurrentScope; }

  void addTask(const CompileScriptTask & task);
  Class addTask(const ClassTemplate & ct, const std::vector<TemplateArgument> & args, const std::shared_ptr<ast::ClassDecl> & class_decl);

protected:
  Type resolve(const ast::QualifiedType & qt);
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & id);

  Function registerRootFunction();
  void processOrCollectScriptDeclarations(const CompileScriptTask & task);
  bool processOrCollectScriptDeclarations();
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const Scope & scp);
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration);
  void collectDeclaration(const std::shared_ptr<ast::Declaration> & decl);
  void resolveIncompleteTypes();
  void processDataMemberDecl(const std::shared_ptr<ast::VariableDecl> & decl);
  void processNamespaceVariableDecl(const std::shared_ptr<ast::VariableDecl> & decl);
  void processFriendDecl(const std::shared_ptr<ast::FriendDeclaration> & decl);
  void processPendingDeclarations();
  bool compileFunctions();
  std::shared_ptr<program::Expression> generateExpression(const std::shared_ptr<ast::Expression> & e);

  void processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & decl);
  void fill(ClassBuilder & builder, const std::shared_ptr<ast::ClassDecl> & decl);
  void readClassContent(Class & c, const std::shared_ptr<ast::ClassDecl> & decl);
  void processEnumDeclaration(const std::shared_ptr<ast::EnumDeclaration> & decl);
  void processTypedef(const std::shared_ptr<ast::Typedef> & decl);
  void processNamespaceDecl(const std::shared_ptr<ast::NamespaceDeclaration> & decl);
  void processImportDirective(const std::shared_ptr<ast::ImportDirective> & decl);

  void processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & decl);
  void processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl);
  void processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl);
  void processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl);
  void processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl);
  void processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl);

  void processTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl);
  std::vector<TemplateParameter> processTemplateParameters(const std::shared_ptr<ast::TemplateDeclaration> & decl);
  void processClassTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl);
  void processFunctionTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl);

  // function-related functions
  void reprocess(const IncompleteFunction & func);

protected:
  void schedule(const Function & f, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp);

protected:
  Class build(const ClassBuilder & builder);
  Function build(const FunctionBuilder & builder);
  Enum build(const Enum &, const std::string & name);

protected:
  class StateGuard
  {
  private:
    ScriptCompiler *compiler;
    Script script;
    std::shared_ptr<ast::AST> ast;
    Scope scope;
  public:
    StateGuard(ScriptCompiler *c);
    StateGuard(const StateGuard &) = delete;
    ~StateGuard();
  };

  friend class StateGuard;

protected:
  Script mCurrentScript;
  std::shared_ptr<ast::AST> mCurrentAst;

  std::vector<ScopedDeclaration> mProcessingQueue; // data members (including static data members), friend declarations

  Scope mCurrentScope;

  std::vector<StaticVariable> mStaticVariables;
  std::vector<CompileFunctionTask> mCompilationTasks;

  VariableProcessor variable_;
  ExpressionCompiler expr_;

  std::vector<IncompleteFunction> mIncompleteFunctions;

  std::vector<CompileScriptTask> mTasks; // we need to store the tasks to maintain the ASTs alive.

  ScriptCompilerNameResolver name_resolver;
  TypeResolver<ScriptCompilerNameResolver> type_resolver;

  typedef BasicPrototypeResolver<LenientTypeResolver<ScriptCompilerNameResolver>> PrototypeResolver;
  FunctionProcessor<PrototypeResolver> function_processor_;

  ScopeStatementProcessor<BasicNameResolver> scope_statements_;

  ImportProcessor<ScriptCompilerModuleLoader> modules_;
};

inline Engine* ScriptCompilerNameResolver::engine() const
{
  return compiler->engine();
}

inline NameLookup ScriptCompilerNameResolver::resolve(const std::shared_ptr<ast::Identifier> & name)
{
  return NameLookup::resolve(name, compiler->currentScope());
}

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SCRIPT_H
