// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_SCRIPT_H
#define LIBSCRIPT_COMPILE_SCRIPT_H

#include "script/compiler/compilercomponent.h"

#include "script/compiler/compilefunctiontask.h"
#include "script/compiler/defaultargumentprocessor.h"
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

class ScriptCompiler;
class ScriptCompilerTemplateNameProcessor;

class ScriptCompilerNameResolver
{
public:
  ScriptCompiler* compiler;
  ScriptCompilerTemplateNameProcessor *tnp;
public:
  ScriptCompilerNameResolver() = default;
  ScriptCompilerNameResolver(const ScriptCompilerNameResolver &) = default;

  inline Engine* engine() const;

  NameLookup resolve(const std::shared_ptr<ast::Identifier> & name);

  NameLookup resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scp);
};

class ScriptCompilerModuleLoader
{
public:
  ScriptCompiler *compiler_;
  Engine* engine() const;

  Script load(const SourceFile &src);
};

struct ScriptCompilerComponentKey
{
private:
  ScriptCompilerComponentKey() = default;
  friend class ScriptCompiler;
  friend class ScriptCompilerModuleLoader;
  friend class ScriptCompilerNameResolver;
};

class ScriptCompiler : public CompilerComponent
{
public:
  ScriptCompiler(Compiler *c);
  ~ScriptCompiler();

  void compile(const Script & task);

  Class instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args);

  inline Script script() const { return mCurrentScript; }
  inline const Scope & currentScope() const { return mCurrentScope; }

public:
  void load(const Script & s, ScriptCompilerComponentKey);
  Class instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args, ScriptCompilerComponentKey);

protected:
  Type resolve(const ast::QualifiedType & qt);
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & id);

  Function registerRootFunction();
  void processOrCollectScriptDeclarations(const Script & task);
  bool processOrCollectScriptDeclarations();
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const Scope & scp);
  void processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration);
  void collectDeclaration(const std::shared_ptr<ast::Declaration> & decl);
  void resolveIncompleteTypes();
  void processFriendDecl(const std::shared_ptr<ast::FriendDeclaration> & decl);
  void processPendingDeclarations();
  bool compileFunctions();

  void processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & decl);
  void fill(ClassBuilder & builder, const std::shared_ptr<ast::ClassDecl> & decl);
  std::string readClassName(const std::shared_ptr<ast::ClassDecl> & decl);
  Type readClassBase(const std::shared_ptr<ast::ClassDecl> & decl);
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
  Namespace findEnclosingNamespace(const Scope & scp) const;
  ClassTemplate findClassTemplate(const std::string & name, const std::vector<Template> & list);
  void processClassTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl);
  void processClassTemplatePartialSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl);
  void processFunctionTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl);

  // function-related functions
  void reprocess(IncompleteFunction & func);

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

  std::vector<ScopedDeclaration> mProcessingQueue; // data members (including static data members), friend declarations

  Scope mCurrentScope;

  std::vector<CompileFunctionTask> mCompilationTasks;

  VariableProcessor variable_;

  std::vector<IncompleteFunction> mIncompleteFunctions;

  ScriptCompilerNameResolver name_resolver;
  TypeResolver<ScriptCompilerNameResolver> type_resolver;
  std::unique_ptr<ScriptCompilerTemplateNameProcessor> tnp_;

  typedef BasicPrototypeResolver<LenientTypeResolver<ScriptCompilerNameResolver>> PrototypeResolver;
  FunctionProcessor<PrototypeResolver> function_processor_;

  ScopeStatementProcessor<BasicNameResolver> scope_statements_;

  ImportProcessor<ScriptCompilerModuleLoader> modules_;

  DefaultArgumentProcessor default_arguments_;
};

inline Engine* ScriptCompilerNameResolver::engine() const
{
  return compiler->engine();
}

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_SCRIPT_H
