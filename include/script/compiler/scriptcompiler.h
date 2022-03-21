// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SCRIPT_H
#define LIBSCRIPT_COMPILE_SCRIPT_H

#include "script/compiler/component.h"
#include "script/compiler/compilefunctiontask.h"
#include "script/compiler/defaultargumentprocessor.h"
#include "script/compiler/expressioncompiler.h"
#include "script/compiler/functionprocessor.h"
#include "script/compiler/importprocessor.h"
#include "script/compiler/scopestatementprocessor.h"
#include "script/compiler/typeresolver.h"
#include "script/compiler/variableprocessor.h"

#include "script/engine.h"
#include "script/functioncreator.h"
#include "script/script.h"
#include "script/templateargumentprocessor.h"
#include "script/types.h"

#include "script/ast/forwards.h"

#include <queue>

namespace script
{

class NameLookup;
class Template;
class TemplateArgument;

namespace compiler
{

class Compiler;

struct ScopedDeclaration
{
  ScopedDeclaration() { }
  ScopedDeclaration(const script::Scope & scp, const std::shared_ptr<ast::Declaration> & decl) : scope(scp), declaration(decl) { }

  Scope scope;
  std::shared_ptr<ast::Declaration> declaration;
};


class ScriptCompiler : Component
{
public:
  explicit ScriptCompiler(Compiler *c);
  ScriptCompiler(const ScriptCompiler&) = delete;
  ~ScriptCompiler();

  void add(const Script & task);
  Class instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args);

  bool done() const;
  void processNext();

  ImportProcessor & importProcessor() { return modules_; }

  inline Script script() const { return mCurrentScript; }
  inline const Scope & currentScope() const { return mCurrentScope; }

  inline std::queue<CompileFunctionTask> & compileTasks() { return mCompilationTasks; }
  VariableProcessor & variableProcessor() { return variable_; }

protected:
  Type resolve(const ast::QualifiedType & qt);
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & id);

  Function registerRootFunction();
  void processOrCollectScriptDeclarations(const Script & task);
  bool processOrCollectScriptDeclarations();
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const Scope & scp);
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration);
  void collectDeclaration(const std::shared_ptr<ast::Declaration> & decl);
  void processFriendDecl(const std::shared_ptr<ast::FriendDeclaration> & decl);
  void processPendingDeclarations();

  int getIdAttribute(const std::shared_ptr<ast::AttributeDeclaration>& attrdecl) const;

  void processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & decl);
  void fill(ClassBuilder & builder, const std::shared_ptr<ast::ClassDecl> & decl);
  std::string readClassName(const std::shared_ptr<ast::ClassDecl> & decl);
  Type readClassBase(const std::shared_ptr<ast::ClassDecl> & decl);
  void readClassContent(Class & c, const std::shared_ptr<ast::ClassDecl> & decl);
  void processEnumDeclaration(const std::shared_ptr<ast::EnumDeclaration> & decl);
  void processTypedef(const std::shared_ptr<ast::Typedef> & decl);
  void processNamespaceDecl(const std::shared_ptr<ast::NamespaceDeclaration> & decl);
  void processImportDirective(const std::shared_ptr<ast::ImportDirective> & decl);

  FunctionCreator& getFunctionCreator(const Script& s);

  void processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & decl);
  void processBasicFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & decl);
  void processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl);
  void processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl);
  void processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl);
  void processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl);
  void processFunctionCallOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl);
  void processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl);
  std::vector<Attribute> computeAttributes(const std::shared_ptr<ast::FunctionDecl>& decl);
  void processAttribute(Function& f, const std::shared_ptr<ast::FunctionDecl>& decl);
  void processAttribute(Function& f, const std::vector<Attribute>& attributes);

  void processTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl);
  std::vector<TemplateParameter> processTemplateParameters(const std::shared_ptr<ast::TemplateDeclaration> & decl);
  void processClassTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl);
  void processFunctionTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl);
  Namespace findEnclosingNamespace(const Scope & scp) const;
  ClassTemplate findClassTemplate(const std::string & name, const std::vector<Template> & list);
  void processClassTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl);
  void processClassTemplatePartialSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl);
  void processFunctionTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl);

  // function-related functions
  void reprocess(ScopedDeclaration & func);

protected:
  void schedule(Function & f, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp);

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
  const std::shared_ptr<ast::AST> & currentAst() const;

protected:
  Script mCurrentScript;

  std::queue<ScopedDeclaration> mProcessingQueue; // data members (including static data members), friend declarations

  Scope mCurrentScope;

  std::queue<CompileFunctionTask> mCompilationTasks;

  VariableProcessor variable_;

  /// TODO: maybe merge this with 'mProcessingQueue'
  std::queue<ScopedDeclaration> mIncompleteFunctionDeclarations;

  FunctionProcessor function_processor_;

  ScopeStatementProcessor scope_statements_;

  ImportProcessor modules_;

  DefaultArgumentProcessor default_arguments_;

  FunctionTemplateProcessor templates_;

  FunctionCreator function_creator_;

  bool mReprocessingIncompleteFunctions;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SCRIPT_H
