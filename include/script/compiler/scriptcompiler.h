// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SCRIPT_H
#define LIBSCRIPT_COMPILE_SCRIPT_H

#include "script/compiler/compiler.h"

#include "script/compiler/expressioncompiler.h"

#include "script/types.h"
#include "script/engine.h"

#include <vector>

namespace script
{

class NameLookup;
class Template;
struct TemplateArgument;

namespace ast
{
class AST;
class CastDecl;
class ClassDecl;
class CompoundStatement;
class ConstructorDecl;
class Declaration;
class DestructorDecl;
class EnumDeclaration;
class Expression;
class FriendDeclaration;
class FunctionDecl;
class Identifier;
class NamespaceAliasDefinition;
class NamespaceDeclaration;
class OperatorOverloadDecl;
class VariableDecl;
class QualifiedType;
class TypeAliasDeclaration;
class Typedef;
class UsingDeclaration;
class UsingDirective;
} // namespace ast

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

  inline Script script() const { return mScript; }
  inline const Scope & currentScope() const { return mCurrentScope; }

protected:
  std::string repr(const std::shared_ptr<ast::Identifier> & id);
  Type resolve(const ast::QualifiedType & qt);
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & id);

  Function registerRootFunction(const Scope & scp);
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

protected:
  void schedule(const CompileFunctionTask & task);

protected:
  Class build(const ClassBuilder & builder);
  Function build(const FunctionBuilder & builder);
  Enum build(const Enum &, const std::string & name);

protected:
  Script mScript;
  std::shared_ptr<ast::AST> mAst;

  std::vector<ScopedDeclaration> mProcessingQueue; // data members (including static data members), friend declarations

  Scope mCurrentScope;

  std::vector<StaticVariable> mStaticVariables;
  std::vector<CompileFunctionTask> mCompilationTasks;

  ExpressionCompiler mExprCompiler;

  std::vector<IncompleteFunction> mIncompleteFunctions;
  bool mResolvedUnknownType;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SCRIPT_H
