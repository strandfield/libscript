// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_FUNCTION_H
#define LIBSCRIPT_COMPILE_FUNCTION_H

#include "script/compiler/compilefunctiontask.h"
#include "script/compiler/expressioncompiler.h"
#include "script/private/functionscope_p.h" 
#include "script/compiler/importprocessor.h"
#include "script/compiler/logger.h"
#include "script/compiler/scopestatementprocessor.h"
#include "script/compiler/stack.h"

#include "script/functiontemplateprocessor.h"

#include "script/types.h"
#include "script/engine.h"

#include <vector>

namespace script
{

namespace program
{
class Expression;
class CompoundStatement;
class Statement;
class JumpStatement;
class IterationStatement;
class SelectionStatement;
class LambdaExpression;
}

namespace compiler
{

class FunctionCompiler;
class FunctionCompilerTemplateProcessor;

class EnterScope
{
public:
  FunctionCompiler * compiler;
  EnterScope(FunctionCompiler *c, FunctionScope::Category scp);
  ~EnterScope();

  void leave();

  EnterScope(const EnterScope &) = delete;
};

class StackVariableAccessor : public VariableAccessor
{
protected:
  Stack * stack_;
public:
  StackVariableAccessor(Stack & s);
  ~StackVariableAccessor() = default;

  inline const Stack & stack() const { return *stack_; }

  std::shared_ptr<program::Expression> accessLocal(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos) override;
};

class FunctionCompilerLambdaProcessor : public LambdaProcessor
{
protected:
  Stack * stack_;
  Script script_;
  FunctionCompiler* fcomp_;
public:
  FunctionCompilerLambdaProcessor(Stack & s, FunctionCompiler* fc);
  ~FunctionCompilerLambdaProcessor() = default;

  inline const Stack & stack() const { return *stack_; }
  inline Script & script() { return script_; }

  std::shared_ptr<program::LambdaExpression> generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le) override;
};

class FunctionCompiler
{
public:
  explicit FunctionCompiler(Engine *e);
  ~FunctionCompiler();

  inline Engine* engine() const { return mEngine; }

  void compile(const CompileFunctionTask & task);

  Script script();

  Class classScope();
  const std::shared_ptr<ast::Declaration> & declaration() const;
  const Function & compiledFunction() const;

  void setFunctionTemplateProcessor(FunctionTemplateProcessor & ftp);
  inline FunctionTemplateProcessor & functionTemplateProcessor() { return *ftp_; }
  ImportProcessor & importProcessor() { return modules_; }
  inline Logger & logger() { return *logger_; }
  void setLogger(Logger & lg);

protected:
  bool canUseThis() const;
  bool isCompilingAnonymousFunction() const;
  std::string argumentName(int index);
  std::shared_ptr<ast::CompoundStatement> bodyDeclaration();

  std::shared_ptr<program::Expression> generate(const std::shared_ptr<ast::Expression> & e);

public:
  struct ScopeKey {
  private:
    friend class EnterScope;
    ScopeKey() = default;
  };
  void enter_scope(FunctionScope::Category scopeType, const ScopeKey &);
  void leave_scope(const ScopeKey &);

protected:
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & name);
  
  Scope breakScope() const;
  Scope continueScope() const;

  std::shared_ptr<program::CompoundStatement> generateBody();

  std::shared_ptr<program::CompoundStatement> generateConstructorHeader();

  std::shared_ptr<program::CompoundStatement> generateDestructorFooter();

  std::shared_ptr<program::CompoundStatement> generateDefaultConstructor();
  std::shared_ptr<program::CompoundStatement> generateCopyConstructor();
  std::shared_ptr<program::CompoundStatement> generateMoveConstructor();
  std::shared_ptr<program::CompoundStatement> generateDestructor();

protected:
  // statements buffer related functions
  typedef std::vector<std::shared_ptr<program::Statement>> buffer_type;
  void write(const std::shared_ptr<program::Statement> & s);
  size_t buffer_size();
  std::vector<std::shared_ptr<program::Statement>> resize_buffer(size_t size);
  std::vector<std::shared_ptr<program::Statement>> read(size_t count);
  std::shared_ptr<program::Statement> read_one();

  class BufferSwap
  {
  private:
    buffer_type *first;
    buffer_type *second;
  public:
    BufferSwap(buffer_type & a, buffer_type & b);
    ~BufferSwap();
    BufferSwap(const BufferSwap &) = delete;
  };

protected:
  // main processing functions
  std::shared_ptr<program::Statement> generate(const std::shared_ptr<ast::Statement> & s);
  std::shared_ptr<program::CompoundStatement> generateCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & compoundStatement, FunctionScope::Category scopeType);
  void process(const std::shared_ptr<ast::Statement> & s);
  void processExitScope(const Scope & scp);
  void generateExitScope(const Scope & scp, std::vector<std::shared_ptr<program::Statement>> & statements);

protected:
  void log(const diagnostic::Message & mssg);
  void log(const CompilerException & exception);

private:
  void processCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & compoundStatement, FunctionScope::Category scopeType);
  void processExpressionStatement(const std::shared_ptr<ast::ExpressionStatement> & es);
  void processForLoop(const std::shared_ptr<ast::ForLoop> & fl);
  void processIfStatement(const std::shared_ptr<ast::IfStatement> & is);
  void processImportDirective(const std::shared_ptr<ast::ImportDirective> & id);
  void processJumpStatement(const std::shared_ptr<ast::JumpStatement> & js);
  virtual void processReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs);
  void processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl);
  void processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, std::nullptr_t);
  void processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, const std::shared_ptr<ast::ConstructorInitialization> & init);
  void processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, const std::shared_ptr<ast::BraceInitialization> & init);
  void processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, const std::shared_ptr<ast::AssignmentInitialization> & init);
  void processVariableCreation(const Type & type, const std::string & name, const std::shared_ptr<program::Expression> & value);
  void processVariableDestruction(const Variable & var);
  void processWhileLoop(const std::shared_ptr<ast::WhileLoop> & whileLoop);

protected:
  friend class FunctionScope;
  friend class FunctionCompilerExtension;

  Engine *mEngine;

  Stack mStack;
  Function mFunction;
  Scope mBaseScope;
  Scope mFunctionArgumentsScope;
  Scope mFunctionBodyScope;
  Scope mCurrentScope;
  std::shared_ptr<ast::Declaration> mDeclaration;

  std::vector<std::shared_ptr<program::Statement>> mBuffer;

  TypeResolver<ExtendedNameResolver> type_;
  ExpressionCompiler expr_;
  FunctionCompilerLambdaProcessor lambda_;
  ScopeStatementProcessor<ExtendedNameResolver> scope_statements_;
  ImportProcessor modules_;

  Logger default_logger_;
  Logger *logger_;

  FunctionTemplateProcessor default_ftp_;
  FunctionTemplateProcessor *ftp_;

private:
  StackVariableAccessor variable_;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_FUNCTION_H
