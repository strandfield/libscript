// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SCRIPT_H
#define LIBSCRIPT_COMPILE_SCRIPT_H

#include "script/compiler/compiler.h"

#include "script/compiler/expressioncompiler.h"

#include "script/types.h"
#include "script/engine.h"

#include "script/ast/forwards.h"

#include <vector>

namespace script
{

class NameLookup;
class Template;
struct TemplateArgument;

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

struct CompileFunctionTask;

struct CompileScriptTask
{
  Script script;
  std::shared_ptr<ast::AST> ast;
};

class ScriptCompiler : public CompilerComponent
{
public:
  ScriptCompiler(Compiler *c, CompileSession *s);

  void compile(const CompileScriptTask & task);

  inline Script script() const { return mCurrentScript; }
  inline const Scope & currentScope() const { return mCurrentScope; }

protected:
  std::string repr(const std::shared_ptr<ast::Identifier> & id);
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
  static bool checkStaticInitialization(const std::shared_ptr<program::Expression> & expr);
  bool initializeStaticVariable(const StaticVariable & svar);
  bool initializeStaticVariables();

  void processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & decl);
  void processEnumDeclaration(const std::shared_ptr<ast::EnumDeclaration> & decl);
  void processTypedef(const std::shared_ptr<ast::Typedef> & decl);
  void processNamespaceDecl(const std::shared_ptr<ast::NamespaceDeclaration> & decl);
  void processUsingDirective(const std::shared_ptr<ast::UsingDirective> & decl);
  void processUsingDeclaration(const std::shared_ptr<ast::UsingDeclaration> & decl);
  void processNamespaceAlias(const std::shared_ptr<ast::NamespaceAliasDefinition> & decl);
  void processTypeAlias(const std::shared_ptr<ast::TypeAliasDeclaration> & decl);
  void processImportDirective(const std::shared_ptr<ast::ImportDirective> & decl);

  static AccessSpecifier getAccessSpecifier(const Scope & scp);
  void handleAccessSpecifier(FunctionBuilder &builder, const Scope & scp);
  void processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & decl);
  void processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl);
  void processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl);
  void processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl);
  void processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl);
  void processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl);

  // function-related functions
  Type optional_resolve(const ast::QualifiedType & qt);
  Prototype functionPrototype(const std::shared_ptr<ast::FunctionDecl> & decl);
  void schedule_for_reprocessing(const std::shared_ptr<ast::FunctionDecl> & decl, const Function & f);
  void reprocess(const IncompleteFunction & func);

  // modules-related functions
  void load_script_module(const std::shared_ptr<ast::ImportDirective> & decl);
  void load_script_module(const support::filesystem::path & p);
  void load_script_module_recursively(const support::filesystem::path & p);
  bool is_loaded(const support::filesystem::path & p, Script & result);

protected:
  void schedule(const CompileFunctionTask & task);

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

  ExpressionCompiler mExprCompiler;

  std::vector<IncompleteFunction> mIncompleteFunctions;
  bool mResolvedUnknownType;

  std::vector<CompileScriptTask> mTasks; // we need to store the tasks to maintain the ASTs alive.
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SCRIPT_H
