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
class NamespaceDeclaration;
class OperatorOverloadDecl;
class VariableDecl;
class QualifiedType;
class Typedef;
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

protected:
  std::string repr(const std::shared_ptr<ast::Identifier> & id);
  Type resolve(const ast::QualifiedType & qt, const Scope & scp);
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & id, const Scope & scp);

  Function registerRootFunction(const Scope & scp);
  bool processFirstOrderDeclarationsAndCollectHigherOrderDeclarations();
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const Scope & scope);
  void processSecondOrderDeclarations();
  void processSecondOrderDeclaration(const ScopedDeclaration & decl);
  void processDataMemberDecl(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp);
  void processNamespaceVariableDecl(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp);
  void processFriendDecl(const std::shared_ptr<ast::FriendDeclaration> & decl, const Scope & scp);
  void processThirdOrderDeclarations();
  bool compileFunctions();
  static bool checkStaticInitialization(const std::shared_ptr<program::Expression> & expr);
  bool initializeStaticVariable(const StaticVariable & svar);
  bool initializeStaticVariables();

  bool isFirstOrderDeclaration(const std::shared_ptr<ast::Declaration> & decl) const;
  bool isSecondOrderDeclaration(const std::shared_ptr<ast::Declaration>& decl) const;
  bool isThirdOrderDeclaration(const std::shared_ptr<ast::Declaration> & decl) const;

  void processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & decl, const Scope & scp);
  void processEnumDeclaration(const std::shared_ptr<ast::EnumDeclaration> & decl, const Scope & scp);
  void processTypedef(const std::shared_ptr<ast::Typedef> & decl, const Scope & scp);
  void processNamespaceDecl(const std::shared_ptr<ast::NamespaceDeclaration> & decl, const Scope & scp);
  void processFirstOrderTemplateDeclaration(const std::shared_ptr<ast::Declaration> & decl, const Scope &);

  static AccessSpecifier getAccessSpecifier(const Scope & scp);
  void handleAccessSpecifier(FunctionBuilder &builder, const Scope & scp);
  void processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & decl, const Scope & scp);
  void processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl, const Scope & scp);
  void processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl, const Scope & scp);
  void processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl, const Scope & scp);
  void processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl, const Scope & scp);
  void processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl, const Scope & scp);
  void processSecondOrderTemplateDeclaration(const std::shared_ptr<ast::Declaration> & decl, const Scope &);

  // template-related functions
  TemplateArgument processTemplateArg(const std::shared_ptr<ast::Expression> & arg);


  // function-related functions
  Prototype functionPrototype(const std::shared_ptr<ast::FunctionDecl> & decl, const Scope & scp);

protected:
  void schedule(const CompileFunctionTask & task);

protected:
  Class build(const ClassBuilder & builder);
  Function build(const FunctionBuilder & builder);
  Enum build(const Enum &, const std::string & name);

protected:
  Script mScript;
  std::shared_ptr<ast::AST> mAst;

  std::vector<ScopedDeclaration> mSecondOrderDeclarations; // functions, operators, casts
  std::vector<ScopedDeclaration> mThirdOrderDeclarations; // data members (including static data members)

  Scope mCurrentScope;

  std::vector<StaticVariable> mStaticVariables;
  std::vector<CompileFunctionTask> mCompilationTasks;

  ExpressionCompiler mExprCompiler;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SCRIPT_H
