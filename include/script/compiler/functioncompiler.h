// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_FUNCTION_H
#define LIBSCRIPT_COMPILE_FUNCTION_H

#include "script/compiler/expressioncompiler.h"

#include "script/types.h"
#include "script/engine.h"

#include <vector>

namespace script
{

namespace ast
{
class AssignmentInitialization;
class AST;
class BraceInitialization;
class CompoundStatement;
class ConstructorInitialization;
class ExpressionStatement;
class Declaration;
class ForLoop;
class FunctionCall;
class FunctionDecl;
class Identifier;
class IfStatement;
class JumpStatement;
class QualifiedType;
class ReturnStatement;
class Statement;
class VariableDecl;
class WhileLoop;
}

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

class Compiler;
class FunctionCompiler;

struct ScopeData;

class FunctionScope
{
public:
  enum Type {
    Invalid = 0,
    FunctionBody = 1,
    IfBody = 2,
    WhileBody = 3,
    ForInit = 4,
    ForBody = 5,
    CompoundStatement = 6,
  };

  FunctionScope(FunctionCompiler *c, Type st);
  ~FunctionScope();

  int sp() const;

protected:
  void enterScope(Type scopeType);
  void leaveScope();

private:
  FunctionCompiler *compiler;
  int mIndex; // index of this scope in the scope stack
};

struct ScopeData
{
  FunctionScope::Type type;
  int offset; // offset in the variable stack
};

typedef std::vector<ScopeData> ScopeStack;


struct Variable
{
  Type type;
  std::string name;
  int index;
  bool global;

  Variable();
  Variable(const Type & t, const std::string & n, int i, bool g = false);
};

class Stack
{
public:
  Stack() : size(0), capacity(0), data(0) { }
  Stack(const Stack &) = delete;
  Stack(int s);
  ~Stack();

  void clear();

  int addVar(const Type & t, const std::string & name);
  int addGlobal(const Type & t, const std::string & name);
  bool exists(const std::string & var) const;
  int indexOf(const std::string & var) const;
  int lastIndexOf(const std::string & var) const;
  void destroy(int n);

  Stack & operator=(const Stack &) = delete;

  const Variable & at(int i) const;
  Variable & operator[](int index);

protected:
  void realloc(int s);

public:
  int size;
  int capacity;
  int max_size;
  Variable *data;
};



struct CompileFunctionTask
{
  CompileFunctionTask() { }
  CompileFunctionTask(const Function & f, const std::shared_ptr<ast::FunctionDecl> & d, const script::Scope & s) :
    function(f), declaration(d), scope(s) { }

  Function function;
  std::shared_ptr<ast::FunctionDecl> declaration;
  script::Scope scope;
};



class FunctionCompiler : public AbstractExpressionCompiler
{
public:
  FunctionCompiler(Compiler *c, CompileSession *s);

  void compile(const CompileFunctionTask & task);


  Script script();
  script::Scope currentScope() const override;
  Class classScope();
  const std::shared_ptr<ast::Declaration> & declaration() const;
  const Function & compiledFunction() const;

protected:
  bool isCompilingAnonymousFunction() const;
  std::string argumentName(int index);
  std::shared_ptr<ast::CompoundStatement> bodyDeclaration();
  std::shared_ptr<ast::Expression> defaultArgumentValue(int index);

  bool canUseThis() const;

  void enterScope(FunctionScope::Type scopeType);
  void leaveScope(int depth = 1);

  int breakDepth();
  int continueDepth();

  NameLookup unqualifiedLookup(const std::shared_ptr<ast::Identifier> & name) override;

  std::vector<Operator> getOperators(Operator::BuiltInOperator op, Type type, int lookup_policy = OperatorLookupPolicy::FetchParentOperators | OperatorLookupPolicy::RemoveDuplicates | OperatorLookupPolicy::ConsiderCurrentScope) override;

  std::shared_ptr<program::Statement> generateStatement(const std::shared_ptr<ast::Statement> & statement);
  void generateStatements(const std::vector<std::shared_ptr<ast::Statement>> & in, std::vector<std::shared_ptr<program::Statement>> & out);

  std::shared_ptr<program::CompoundStatement> generateBody();

  std::shared_ptr<program::Expression> generateDefaultArgument(int index);

  std::shared_ptr<program::Expression> defaultConstructMember(const Type & t);
  std::shared_ptr<program::Expression> constructDataMember(const Type & t, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::CompoundStatement> generateConstructorHeader();
  std::shared_ptr<program::Statement> generateDelegateConstructorCall(std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateParentConstructorCall(std::vector<std::shared_ptr<program::Expression>> & args);

  std::shared_ptr<program::CompoundStatement> generateDestructorFooter();
  Function getDestructor(const Type & t);

  std::shared_ptr<program::CompoundStatement> generateDefaultConstructor();
  std::shared_ptr<program::CompoundStatement> generateCopyConstructor();
  std::shared_ptr<program::CompoundStatement> generateMoveConstructor();
  std::shared_ptr<program::CompoundStatement> generateDestructor();

  std::shared_ptr<program::CompoundStatement> generateAssignmentOperator();
  bool isAssignmentOperator(const Operator & op, const Type & t) const;
  Operator findAssignmentOperator(const Type & t);

  std::shared_ptr<program::LambdaExpression> generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr) override;
  std::shared_ptr<program::Expression> generateCall(const std::shared_ptr<ast::FunctionCall> & call) override;
  std::shared_ptr<program::CompoundStatement> generateCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & compoundStatement, compiler::FunctionScope::Type scopeType);
  std::shared_ptr<program::Statement> generateExpressionStatement(const std::shared_ptr<ast::ExpressionStatement> & es);
  std::shared_ptr<program::Statement> generateForLoop(const std::shared_ptr<ast::ForLoop> & forLoop);
  std::shared_ptr<program::Statement> generateIfStatement(const std::shared_ptr<ast::IfStatement> & ifStatement);
  std::shared_ptr<program::Statement> generateJumpStatement(const std::shared_ptr<ast::JumpStatement> & js);
  virtual std::shared_ptr<program::Statement> generateReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs);
  void generateScopeDestruction(int depth, std::vector<std::shared_ptr<program::Statement>> & statements);
  void generateScopeDestruction(compiler::FunctionScope & scp, std::vector<std::shared_ptr<program::Statement>> & statements);
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier);
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup) override;
  std::shared_ptr<program::Expression> generateThisAccess();
  std::shared_ptr<program::Expression> generateMemberAccess(const int index);
  std::shared_ptr<program::Expression> generateMemberAccess(const std::shared_ptr<program::Expression> & object, const int index);
  std::shared_ptr<program::Expression> generateGlobalAccess(int index);
  std::shared_ptr<program::Expression> generateLocalVariableAccess(int index);
  std::shared_ptr<program::Statement> generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl);
  std::shared_ptr<program::Statement> generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, std::nullptr_t);
  std::shared_ptr<program::Statement> generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, const std::shared_ptr<ast::ConstructorInitialization> & init);
  std::shared_ptr<program::Statement> generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, const std::shared_ptr<ast::BraceInitialization> & init);
  std::shared_ptr<program::Statement> generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & varDecl, const Type & var_type, const std::shared_ptr<ast::AssignmentInitialization> & init);
  std::shared_ptr<program::Expression> constructFundamentalValue(const Type & t, bool copy);
  std::shared_ptr<program::Statement> generateFundamentalVariableCreation(const Type & type, const std::string & name);
  std::shared_ptr<program::Statement> generateVariableCreation(const Type & type, const std::string & name, const std::shared_ptr<program::Expression> & value);
  std::shared_ptr<program::Statement> generateVariableDestruction(const Variable & var);
  std::shared_ptr<program::Statement> generateVariableInitialization(const Variable & var);
  std::shared_ptr<program::Statement> generateVariableInitialization(const Variable & var, const std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateVariableInitialization(const std::shared_ptr<program::Expression> & var, int type, const std::vector<std::shared_ptr<program::Expression>> & args);
  std::shared_ptr<program::Statement> generateWhileLoop(const std::shared_ptr<ast::WhileLoop> & whileLoop);

protected:
  friend class FunctionScope;

  Script mScript;

  Stack mStack;
  ScopeStack mScopeStack;
  Function mFunction;
  script::Scope mCurrentScope;
  std::shared_ptr<ast::Declaration> mDeclaration;
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_FUNCTION_H
