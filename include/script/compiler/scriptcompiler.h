// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SCRIPT_H
#define LIBSCRIPT_COMPILE_SCRIPT_H

#include "script/compiler/compiler.h"

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
class CompoundStatement;
class Declaration;
class Expression;
class FunctionDecl;
class Identifier;
class VariableDecl;
class QualifiedType;
} // namespace ast

namespace compiler
{

class Compiler;

struct StaticVariable
{
  StaticVariable() { }
  StaticVariable(const Value & var, const std::shared_ptr<ast::Expression> & ie, const script::Scope & scp) :
    variable(var), init(ie), scope(scp) { }

  Value variable;
  std::shared_ptr<ast::Expression> init;
  script::Scope scope;
};


struct Declaration
{
  Declaration() { }
  Declaration(const script::Scope & scp, const std::shared_ptr<ast::Declaration> & decl) : scope(scp), declaration(decl) { }

  script::Scope scope;
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

  struct StatePasskey
  {
  private:
    friend class StateLock;
    StatePasskey() {}
  };

  inline const script::Scope & currentScope() const { return mCurrentScope; }
  inline void setCurrentScope(script::Scope scope, const StatePasskey &) { setCurrentScope(scope); }
  inline const std::shared_ptr<ast::Declaration> & currentDeclaration() const { return mCurrentDeclaration; }
  inline void setCurrentDeclaration(const std::shared_ptr<ast::Declaration> & decl, const StatePasskey &) { setCurrentDeclaration(decl); }

protected:
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & identifier);
  std::string repr(const std::shared_ptr<ast::Identifier> & id); /// TODO : merge this duplicate of AbstractExpressionCompiler::repr()
  Type resolveFunctionType(const ast::QualifiedType & qt);
  Type resolve(const ast::QualifiedType & qt);

  Function registerRootFunction();
  bool processFirstOrderDeclarationsAndCollectHigherOrderDeclarations();
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const script::Scope & scope);
  void processSecondOrderDeclarations();
  bool processSecondOrderDeclaration(const compiler::Declaration & decl);
  void processDataMemberDecl(const std::shared_ptr<ast::VariableDecl> & decl);
  void processThirdOrderDeclarations();
  bool compileFunctions();
  static bool checkStaticInitialization(const std::shared_ptr<program::Expression> & expr);
  bool initializeStaticVariable(const StaticVariable & svar);
  bool initializeStaticVariables();

  bool isFirstOrderDeclaration(const std::shared_ptr<ast::Declaration> & decl) const;
  bool isSecondOrderDeclaration(const std::shared_ptr<ast::Declaration>& decl) const;
  bool isThirdOrderDeclaration(const std::shared_ptr<ast::Declaration> & decl) const;

  bool processClassDeclaration();
  void processEnumDeclaration();
  void processTypedef();
  bool processFirstOrderTemplateDeclaration();

  bool processFunctionDeclaration();
  bool processConstructorDeclaration();
  bool processDestructorDeclaration();
  bool processLiteralOperatorDecl();
  bool processOperatorOverloadingDeclaration();
  bool processCastOperatorDeclaration();
  bool processSecondOrderTemplateDeclaration();

  // template-related functions
  TemplateArgument processTemplateArg(const std::shared_ptr<ast::Expression> & arg);


  // function-related functions
  Prototype functionPrototype();

protected:
  void setCurrentScope(script::Scope scope);
  void setCurrentDeclaration(const std::shared_ptr<ast::Declaration> & decl);
  void setState(const script::Scope & scope, const std::shared_ptr<ast::Declaration> & decl);

  void schedule(const CompileFunctionTask & task);

protected:
  Class build(const ClassBuilder & builder);
  Function build(const FunctionBuilder & builder);
  Enum build(const Enum &, const std::string & name);

protected:
  Script mScript;
  std::shared_ptr<ast::AST> mAst;

  std::vector<Declaration> mSecondOrderDeclarations; // functions, operators, casts
  std::vector<Declaration> mThirdOrderDeclarations; // data members (including static data members)

  script::Scope mCurrentScope;
  std::shared_ptr<ast::Declaration> mCurrentDeclaration;

  std::vector<StaticVariable> mStaticVariables;
  std::vector<CompileFunctionTask> mCompilationTasks;
};

class StateLock
{
public:
  StateLock(ScriptCompiler *c);
  StateLock(const StateLock &) = delete;
  ~StateLock();

  StateLock & operator=(const StateLock &) = delete;

private:
  ScriptCompiler *compiler;
  script::Scope scope;
  std::shared_ptr<ast::Declaration> decl;
};

} // namespace compiler


} // namespace script


#endif // LIBSCRIPT_COMPILE_SCRIPT_H
