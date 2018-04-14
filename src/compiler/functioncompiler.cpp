// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/functioncompiler.h"
#include "functionscope_p.h"

#include "script/compiler/compilererrors.h"

#include "script/compiler/assignmentcompiler.h"
#include "script/compiler/constructorcompiler.h"
#include "script/compiler/destructorcompiler.h"
#include "script/compiler/lambdacompiler.h"

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/cast.h"
#include "../function_p.h"
#include "script/functiontype.h"
#include "script/lambda.h"
#include "script/literals.h"
#include "script/namelookup.h"
#include "../namelookup_p.h"
#include "script/overloadresolution.h"
#include "../script_p.h"

namespace script
{

namespace compiler
{

FunctionScope::FunctionScope(FunctionCompiler *fc, Category cat, Scope p)
  : ExtensibleScope(p.impl())
  , mCompiler(fc)
  , mCategory(cat)
{
  mSize = 0;
  mSp = mCompiler->mStack.size;
}

FunctionScope::FunctionScope(const FunctionScope & other)
  : ExtensibleScope(other)
  , mCompiler(other.mCompiler)
  , mCategory(other.mCategory)
{

}

Engine * FunctionScope::engine() const
{
  return mCompiler->engine();
}

int FunctionScope::kind() const
{
  return Scope::FunctionScope;
}

FunctionScope * FunctionScope::clone() const
{
  return new FunctionScope(*this);
}


const std::vector<Class> & FunctionScope::classes() const
{
  static const std::vector<Class> dummy = std::vector<Class>{};
  return dummy;
}

const std::vector<Enum> & FunctionScope::enums() const
{
  static const std::vector<Enum> dummy = std::vector<Enum>{};
  return dummy;
}

const std::vector<Function> & FunctionScope::functions() const
{
  static const std::vector<Function> dummy = std::vector<Function>{};
  return dummy;
}

const std::vector<LiteralOperator> & FunctionScope::literal_operators() const
{
  static const std::vector<LiteralOperator> dummy = std::vector<LiteralOperator>{};
  return dummy;
}

const std::vector<Namespace> & FunctionScope::namespaces() const
{
  static const std::vector<Namespace> dummy = std::vector<Namespace>{};
  return dummy;
}

const std::vector<Operator> & FunctionScope::operators() const
{
  static const std::vector<Operator> dummy = std::vector<Operator>{};
  return dummy;
}

const std::vector<Template> & FunctionScope::templates() const
{
  static const std::vector<Template> dummy = std::vector<Template>{};
  return dummy;
}

bool FunctionScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  if (name == "this")
  {
    if (!mCompiler->canUseThis())
      throw IllegalUseOfThis{ };

    nl->localIndex = 1;
    return true;
  }

  for (int i(mSp + mSize - 1); i >= mSp; --i)
  {
    if (mCompiler->mStack[i].name == name)
    {
      nl->localIndex = i;
      return true;
    }
  }

  return false;
}

int FunctionScope::add_var(const std::string & name, const Type & t, bool global)
{
  int stack_index;

  if (global)
    stack_index = mCompiler->mStack.addGlobal(t, name);
  else
    stack_index = mCompiler->mStack.addVar(t, name);

  mSize++;

  return stack_index;
}

void FunctionScope::destroy()
{
  mCompiler->mStack.destroy(mSize);
  mSize = 0;
}


FunctionScope::Category FunctionScope::category() const
{
  return mCategory;
}

bool FunctionScope::catch_break() const
{
  return mCategory == ForInit || mCategory == WhileBody;
}


Variable::Variable()
  : index(-1)
  , global(false)
{

}

Variable::Variable(const Type & t, const std::string & n, int i, bool g)
  : type(t)
  , name(n)
  , index(i)
  , global(g)
{

}

Stack::Stack(int s) : size(0), capacity(s), max_size(0)
{
  this->data = new Variable[s];
}

Stack::~Stack()
{
  this->clear();
}


void Stack::clear()
{
  if (this->data)
  {
    delete[] this->data;
    this->data = 0;
  }

  this->size = 0;
  this->capacity = 0;
  this->max_size = 0;
}

int Stack::addVar(const Type & t, const std::string & name)
{
  if (this->size == this->capacity)
  {
    this->realloc((this->size + 1) * 2);
  }

  this->data[this->size] = Variable{ t, name, this->size };
  this->size += 1;
  if (this->size > this->max_size)
  {
    this->max_size = this->size;
  }
  return this->size - 1;
}

int Stack::addGlobal(const Type & t, const std::string & name)
{
  const int offset = addVar(t, name);
  this->data[offset].global = true;
  return offset;
}

bool Stack::exists(const std::string & var) const
{
  return indexOf(var) != -1;
}

int Stack::indexOf(const std::string & var) const
{
  for (int i(0); i < this->size; ++i)
  {
    if (this->data[i].name == var)
      return i;
  }

  return -1;
}

int Stack::lastIndexOf(const std::string & var) const
{
  for (int i(this->size - 1); i >= 0; --i)
  {
    if (this->data[i].name == var)
      return i;
  }

  return -1;
}

void Stack::destroy(int n)
{
  if (n > this->size)
    n = this->size;

  while (n > 0)
  {
    this->data[this->size - 1] = Variable{};
    this->size -= 1;
    n--;
  }
}


const Variable & Stack::at(int i) const
{
  return this->data[i];
}

Variable & Stack::operator[](int i)
{
  return this->data[i];
}

void Stack::realloc(int s)
{
  const int former_size = (this->size > s ? s : this->size);
  Variable *old_vars = this->data;

  this->data = new Variable[s];

  if (old_vars)
  {
    int i(0);
    while (i < former_size)
    {
      this->data[i] = old_vars[i];
      ++i;
    }

    delete[] old_vars;
  }

  this->size = this->size > s ? s : this->size;
  this->capacity = s;
}



FunctionCompiler::FunctionCompiler(Compiler *c, CompileSession *s)
  : AbstractExpressionCompiler(c, s)
{

}


void FunctionCompiler::compile(const CompileFunctionTask & task)
{
  mScript = task.function.script();
  
  mFunction = task.function;
  mDeclaration = task.declaration;
  mBaseScope = task.scope;
  mCurrentScope = task.scope;

  mStack.clear();

  const Prototype & proto = mFunction.prototype();
  if(!mFunction.isDestructor())
    mStack.addVar(proto.returnType(), "return-value");

  enterScope(FunctionScope::FunctionArguments);
  for (int i(0); i < proto.argc(); ++i)
    std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(argumentName(i), proto.argv(i));

  std::shared_ptr<program::CompoundStatement> body = generateBody();
  /// TODO : add implicit return statement in void functions

  std::vector<std::shared_ptr<program::Statement>> default_arg_inits;
  if (mFunction.prototype().hasDefaultArgument())
  {
    const int default_arg_count = mFunction.prototype().defaultArgCount();
    for (int i(mFunction.prototype().argc() - default_arg_count); i < mFunction.prototype().argc(); ++i)
    {
      auto val = generateDefaultArgument(i);
      default_arg_inits.push_back(program::PushDefaultArgument::New(i, val));
    }
  }
  body->statements.insert(body->statements.begin(), default_arg_inits.begin(), default_arg_inits.end());
  
  leaveScope();

  mFunction.implementation()->set_impl(body);
}


Script FunctionCompiler::script()
{
  return mScript;
}

script::Scope FunctionCompiler::scope() const
{
  return mCurrentScope;
}

Class FunctionCompiler::classScope()
{
  return mBaseScope.asClass();
}

const std::shared_ptr<ast::Declaration> & FunctionCompiler::declaration() const
{
  return mDeclaration;
}

const Function & FunctionCompiler::compiledFunction() const
{
  return mFunction;
}

bool FunctionCompiler::isCompilingAnonymousFunction() const
{
  return dynamic_cast<const ScriptFunctionImpl*>(compiledFunction().implementation()) != nullptr;
}

std::string FunctionCompiler::argumentName(int index)
{
  auto funcdecl = std::dynamic_pointer_cast<ast::FunctionDecl>(declaration());

  if (mFunction.isMemberFunction())
  {
    if (index == 0)
      return "this";
    else
      return funcdecl->parameterName(index - 1);
  }

  return funcdecl->parameterName(index);
}

std::shared_ptr<ast::CompoundStatement> FunctionCompiler::bodyDeclaration()
{
  auto funcdecl = std::dynamic_pointer_cast<ast::FunctionDecl>(declaration());
  return funcdecl->body;
}

std::shared_ptr<ast::Expression> FunctionCompiler::defaultArgumentValue(int index)
{
  auto funcdecl = std::dynamic_pointer_cast<ast::FunctionDecl>(declaration());
  return funcdecl->params.at(index).defaultValue;
}


bool FunctionCompiler::canUseThis() const
{
  return mFunction.isMemberFunction() || mFunction.isConstructor() || mFunction.isDestructor();
}


void FunctionCompiler::enterScope(FunctionScope::Category scopeType)
{
  mCurrentScope = Scope{ std::make_shared<FunctionScope>(this, scopeType, mCurrentScope) };
  if (scopeType == FunctionScope::FunctionBody)
    mFunctionBodyScope = mCurrentScope;
  else if (scopeType == FunctionScope::FunctionArguments)
    mFunctionArgumentsScope = mCurrentScope;
}

void FunctionCompiler::leaveScope(int depth)
{
  for (int i(0); i < depth; ++i)
  {
    std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->destroy();
    mCurrentScope = mCurrentScope.parent();
  }
}

Scope FunctionCompiler::breakScope() const
{
  Scope s = mCurrentScope;
  while (!std::dynamic_pointer_cast<FunctionScope>(s.impl())->catch_break())
    s = s.parent();
  return s;
}

std::shared_ptr<program::Statement> FunctionCompiler::generateStatement(const std::shared_ptr<ast::Statement> & statement)
{
  switch (statement->type())
  {
  case ast::NodeType::NullStatement:
    return nullptr; /// TODO : should we create a program::Statement for this
  case ast::NodeType::IfStatement:
    return generateIfStatement(std::dynamic_pointer_cast<ast::IfStatement>(statement));
  case ast::NodeType::WhileLoop:
    return generateWhileLoop(std::dynamic_pointer_cast<ast::WhileLoop>(statement));
  case ast::NodeType::ForLoop:
    return generateForLoop(std::dynamic_pointer_cast<ast::ForLoop>(statement));
  case ast::NodeType::CompoundStatement:
    return generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(statement), FunctionScope::CompoundStatement);
  case ast::NodeType::VariableDeclaration:
    return generateVariableDeclaration(std::dynamic_pointer_cast<ast::VariableDecl>(statement));
  case ast::NodeType::BreakStatement:
  case ast::NodeType::ContinueStatement:
  case ast::NodeType::ReturnStatement:
    return generateJumpStatement(std::dynamic_pointer_cast<ast::JumpStatement>(statement));
  case ast::NodeType::ExpressionStatement:
    return generateExpressionStatement(std::dynamic_pointer_cast<ast::ExpressionStatement>(statement));
  default:
    break;
  }

  assert(false);
  throw std::runtime_error{ "FunctionCompiler::generateStatement() : not implemented" };
}

void FunctionCompiler::generateStatements(const std::vector<std::shared_ptr<ast::Statement>> & in, std::vector<std::shared_ptr<program::Statement>> & out)
{
  for (const auto& s : in)
    out.push_back(generateStatement(s));
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateBody()
{
  if (!mFunction.isDefaulted())
  {
    auto body = this->generateCompoundStatement(bodyDeclaration(), FunctionScope::FunctionBody);

    if (mFunction.isConstructor())
    {
      enterScope(FunctionScope::FunctionBody);
      auto constructor_header = generateConstructorHeader();
      body->statements.insert(body->statements.begin(), constructor_header->statements.begin(), constructor_header->statements.end());
      leaveScope();
    }
    else if (mFunction.isDestructor())
    {
      enterScope(FunctionScope::FunctionBody);
      auto destructor_footer = generateDestructorFooter();
      body->statements.insert(body->statements.end(), destructor_footer->statements.begin(), destructor_footer->statements.end());
      leaveScope();
    }
    else if (isCompilingAnonymousFunction())
    {
      enterScope(FunctionScope::FunctionBody);
      /// TODO : should we generate a dummy program::ReturnStatement ?
      leaveScope();
    }
    
    return body;
  }

  if (mFunction.isDefaultConstructor())
    return generateDefaultConstructor();
  else if (mFunction.isCopyConstructor())
    return generateCopyConstructor();
  else if (mFunction.isMoveConstructor())
    return generateMoveConstructor();
  else if (mFunction.isDestructor())
    return generateDestructor();

  if (mFunction.isOperator() && mFunction.isMemberFunction())
  {
    Operator op = mFunction.toOperator();
    if (op.returnType() == Type::ref(op.memberOf().id())
      && op.prototype().argv(0) == Type::ref(op.memberOf().id())
      && op.prototype().argv(1) == Type::cref(op.memberOf().id()))
      return AssignmentCompiler{ this }.generateAssignmentOperator();
  }

  throw FunctionCannotBeDefaulted{ dpos(mDeclaration) };
}

std::shared_ptr<program::Expression> FunctionCompiler::generateDefaultArgument(int index)
{
  auto fdecl = std::dynamic_pointer_cast<ast::FunctionDecl>(mDeclaration);
  const int param_offset = mFunction.isMemberFunction() ? 1 : 0;
  auto expr = generateExpression(fdecl->params.at(index - param_offset).defaultValue);

  ConversionSequence conv = ConversionSequence::compute(expr, mFunction.prototype().argv(index), engine());
  if (conv == ConversionSequence::NotConvertible())
    throw NotImplementedError{ "FunctionCompiler::generateDefaultArgument() : failed to convert default value" };

  return prepareFunctionArgument(expr, mFunction.prototype().argv(index), conv);
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateConstructorHeader()
{
  return ConstructorCompiler{ this }.generateHeader();
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateDestructorFooter()
{
  return DestructorCompiler{ this }.generateFooter();
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateDefaultConstructor()
{
  return generateConstructorHeader();
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateCopyConstructor()
{
  return ConstructorCompiler{ this }.generateCopyConstructor();
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateMoveConstructor()
{
  return ConstructorCompiler{ this }.generateMoveConstructor();
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateDestructor()
{
  return generateDestructorFooter();
}

std::shared_ptr<program::LambdaExpression> FunctionCompiler::generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr)
{
  CompileLambdaTask task;
  task.lexpr = lambda_expr;
  task.scope = scope();

  const int first_capturable = mFunction.isDestructor() || mFunction.isConstructor() ? 0 : 1;
  LambdaCompiler::preprocess(task, this, mStack, first_capturable);

  std::shared_ptr<LambdaCompiler> compiler{ getComponent<LambdaCompiler>() };
  LambdaCompilationResult result = compiler->compile(task);

  return result.expression;
}

std::shared_ptr<program::Expression> FunctionCompiler::generateCall(const std::shared_ptr<ast::FunctionCall> & call)
{
  const auto & callee = call->callee;
  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(call->arguments);

  if (callee->is<ast::Identifier>()) // this may implicitly call a member
  {
    const std::shared_ptr<ast::Identifier> callee_name = std::static_pointer_cast<ast::Identifier>(callee);
    NameLookup lookup = NameLookup::resolve(callee_name, args, this);

    if (lookup.resultType() == NameLookup::FunctionName)
    {
      std::shared_ptr<program::Expression> object = nullptr;
      if (canUseThis())
        object = program::StackValue::New(1, Type::ref(mStack[1].type));

      OverloadResolution resol = OverloadResolution::New(engine());
      if (!resol.process(lookup.functions(), args, object))
        throw CouldNotFindValidMemberFunction{ dpos(call) };

      Function selected = resol.selectedOverload();
      if (selected.isDeleted())
        throw CallToDeletedFunction{ dpos(call) };
      else if (!Accessibility::check(caller(), selected))
        throw InaccessibleMember{ dpos(call), dstr(callee_name), dstr(selected.accessibility()) };

      const auto & convs = resol.conversionSequence();
     if (selected.isMemberFunction() && !selected.isConstructor())
        args.insert(args.begin(), object);
      prepareFunctionArguments(args, selected.prototype(), convs);
      if (selected.isConstructor()) /// TODO : can this happen ?
        return program::ConstructorCall::New(selected, std::move(args));
      else if (selected.isVirtual() && callee->type() == ast::NodeType::SimpleIdentifier)
        return generateVirtualCall(call, selected, std::move(args));
      return program::FunctionCall::New(selected, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::VariableName || lookup.resultType() == NameLookup::GlobalName
      || lookup.resultType() == NameLookup::DataMemberName || lookup.resultType() == NameLookup::LocalName)
    {
      auto functor = generateVariableAccess(std::dynamic_pointer_cast<ast::Identifier>(callee), lookup);
      return generateFunctorCall(call, functor, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::TypeName)
    {
      return generateConstructorCall(call, lookup.typeResult(), std::move(args));
    }
    
    /// TODO : add error when name does not refer to anything
    throw NotImplementedError{"FunctionCompiler::generateCall() : callee not handled yet"};
  }
  else
    return AbstractExpressionCompiler::generateCall(call);
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & compoundStatement, FunctionScope::Category scopeType)
{
  auto ret = program::CompoundStatement::New();

  ScopeGuard guard{ mCurrentScope };
  enterScope(scopeType);

  for (const auto & s : compoundStatement->statements)
  {
    ret->statements.push_back(generateStatement(s));
  }

  generateExitScope(mCurrentScope, ret->statements);

  return ret;
}

std::shared_ptr<program::Statement> FunctionCompiler::generateExpressionStatement(const std::shared_ptr<ast::ExpressionStatement> & es)
{
  auto expr = generateExpression(es->expression);
  return program::ExpressionStatement::New(expr);
}

std::shared_ptr<program::Statement> FunctionCompiler::generateForLoop(const std::shared_ptr<ast::ForLoop> & forLoop)
{
  ScopeGuard guard{ mCurrentScope };
  enterScope(FunctionScope::ForInit);

  std::shared_ptr<program::Statement> for_init = nullptr;
  if (forLoop->initStatement != nullptr)
    for_init = generateStatement(forLoop->initStatement);

  std::shared_ptr<program::Expression> for_cond = nullptr;
  if (forLoop->condition == nullptr)
    for_cond = program::Literal::New(engine()->newBool(true));
  else
    for_cond = generateExpression(forLoop->condition);

  if (for_cond->type().baseType() != Type::Boolean)
  {
    /// TODO : simply perform an implicit conversion
    throw NotImplementedError{ "Implicit conversion to bool not implemented yet in for-condition" };
  }

  std::shared_ptr<program::Expression> for_loop_incr = nullptr;
  if (forLoop->loopIncrement == nullptr)
    for_loop_incr = program::Literal::New(engine()->newBool(true)); /// TODO : what should we do ? (perhaps use a statement instead, an use the null statement)
  else
    for_loop_incr = generateExpression(forLoop->loopIncrement);

  std::shared_ptr<program::Statement> body = nullptr;
  if (forLoop->body->is<ast::CompoundStatement>())
  {
    body = this->generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(forLoop->body), FunctionScope::ForBody);
  }
  else
  {
    body = generateStatement(forLoop->body);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  generateExitScope(mCurrentScope, statements);

  return program::ForLoop::New(for_init, for_cond, for_loop_incr, body, program::CompoundStatement::New(std::move(statements)));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateIfStatement(const std::shared_ptr<ast::IfStatement> & is)
{
  auto cond = generateExpression(is->condition);
  std::shared_ptr<program::Statement> body;
  if (is->body->is<ast::CompoundStatement>())
    body = generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(is->body), FunctionScope::IfBody);
  else
    body = generateStatement(is->body);

  auto result = program::IfStatement::New(cond, body);

  if (is->elseClause != nullptr)
  {
    result->elseClause = generateStatement(is->elseClause);
  }

  return result;
}

std::shared_ptr<program::Statement> FunctionCompiler::generateJumpStatement(const std::shared_ptr<ast::JumpStatement> & js)
{
  if (js->is<ast::ReturnStatement>())
    return generateReturnStatement(std::dynamic_pointer_cast<ast::ReturnStatement>(js));

  std::vector<std::shared_ptr<program::Statement>> statements;

  const Scope scp = breakScope();
  generateExitScope(scp, statements);

  if (js->is<ast::BreakStatement>())
    return program::BreakStatement::New(std::move(statements));
  else if (js->is<ast::ContinueStatement>())
    return program::ContinueStatement::New(std::move(statements));

  assert(false);
  throw NotImplementedError{ dpos(js), "This kind of jump statement not implemented" };
}

std::shared_ptr<program::Statement> FunctionCompiler::generateReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  generateExitScope(mFunctionBodyScope, statements);

  if (rs->expression == nullptr)
  {
    if (mFunction.prototype().returnType() != Type::Void)
      throw ReturnStatementWithoutValue{};

    return program::ReturnStatement::New(nullptr, std::move(statements));
  }
  else
  {
    if (mFunction.prototype().returnType() == Type::Void)
      throw ReturnStatementWithValue{};
  }

  auto retval = generateExpression(rs->expression);

  const ConversionSequence conv = ConversionSequence::compute(retval, mFunction.prototype().returnType(), engine());

  /// TODO : write a dedicated function for this, don't use prepareFunctionArg()
  retval = prepareFunctionArgument(retval, mFunction.prototype().returnType(), conv);

  return program::ReturnStatement::New(retval, std::move(statements));
}

void FunctionCompiler::generateExitScope(const Scope & scp, std::vector<std::shared_ptr<program::Statement>> & statements)
{
  const int sp = std::dynamic_pointer_cast<FunctionScope>(scp.impl())->sp();
  Stack & stack = mStack;

  for (int i(stack.size - 1); i >= sp; --i)
    statements.push_back(generateVariableDestruction(stack.at(i)));

  // destroying default arguments
  if (std::dynamic_pointer_cast<FunctionScope>(scp.impl())->category() == FunctionScope::FunctionBody)
  {
    const int default_count = mFunction.prototype().defaultArgCount();
    for (int i(mFunction.prototype().argc() - 1); i >= mFunction.prototype().argc() - default_count; --i)
    {
      const Type & arg_type = mFunction.prototype().argv(i);
      const bool destroy = !(arg_type.isReference() || arg_type.isRefRef());
      statements.push_back(program::PopDefaultArgument::New(i, destroy));
    }
  }
}

std::shared_ptr<program::Expression> FunctionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier)
{
  return generateVariableAccess(identifier, resolve(identifier));
}

std::shared_ptr<program::Expression> FunctionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  switch (lookup.resultType())
  {
  case NameLookup::FunctionName:
    return generateFunctionAccess(identifier, lookup);
  case NameLookup::TemplateName:
    throw TemplateNamesAreNotExpressions{ dpos(identifier) };
  case NameLookup::TypeName:
    throw TypeNameInExpression{ dpos(identifier) };
  case NameLookup::VariableName:
    return program::Literal::New(lookup.variable()); /// TODO : perhaps a VariableAccess would be better
  case NameLookup::StaticDataMemberName:
    return generateStaticDataMemberAccess(identifier, lookup);
  case NameLookup::DataMemberName:
    return generateMemberAccess(lookup.dataMemberIndex(), dpos(identifier));
  case NameLookup::GlobalName:
    return generateGlobalAccess(lookup.globalIndex());
  case NameLookup::LocalName:
    return generateLocalVariableAccess(lookup.localIndex());
  case NameLookup::EnumValueName:
    return program::Literal::New(Value::fromEnumValue(lookup.enumValueResult()));
  case NameLookup::NamespaceName:
    throw NamespaceNameInExpression{ dpos(identifier) };
  default:
    break;
  }

  throw NotImplementedError{ "FunctionCompiler::generateVariableAccess() : kind of name not supported yet" };
}

std::shared_ptr<program::Expression> FunctionCompiler::generateThisAccess()
{
  /// TODO : add correct const-qualification
  if(mFunction.isDestructor() || mFunction.isConstructor())
    return program::StackValue::New(0, Type::ref(mStack[0].type));
  return program::StackValue::New(1, Type::ref(mStack[1].type));
}

std::shared_ptr<program::Expression> FunctionCompiler::generateMemberAccess(const int index, const diagnostic::pos_t dpos)
{
  return AbstractExpressionCompiler::generateMemberAccess(generateThisAccess(), index, dpos);
}

std::shared_ptr<program::Expression> FunctionCompiler::generateGlobalAccess(int index)
{
  Script s = mScript;
  auto simpl = s.implementation();
  const Type & gtype = simpl->global_types[index];

  return program::FetchGlobal::New(gtype, index);
}

std::shared_ptr<program::Expression> FunctionCompiler::generateLocalVariableAccess(int index)
{
  const Type t = mStack[index].type;
  return program::StackValue::New(index, t);
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl)
{
  const Type var_type = resolve(var_decl->variable_type);

  if (var_decl->init == nullptr)
    return generateVariableDeclaration(var_decl, var_type, nullptr);
  
  assert(var_decl->init != nullptr);

  if (var_decl->init->is<ast::AssignmentInitialization>())
    return generateVariableDeclaration(var_decl, var_type, std::dynamic_pointer_cast<ast::AssignmentInitialization>(var_decl->init));
  else if (var_decl->init->is<ast::ConstructorInitialization>())
    return generateVariableDeclaration(var_decl, var_type, std::dynamic_pointer_cast<ast::ConstructorInitialization>(var_decl->init));
  else
    return generateVariableDeclaration(var_decl, var_type, std::dynamic_pointer_cast<ast::BraceInitialization>(var_decl->init));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, std::nullptr_t)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ dpos(var_decl) };
  
  try
  {
    return generateVariableCreation(var_type, var_decl->name->getName(), constructValue(var_type, nullptr, dpos(var_decl)));
  }
  catch (const EnumerationsCannotBeDefaultConstructed & e)
  {
    throw EnumerationsMustBeInitialized{ e.pos };
  }
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::ConstructorInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ dpos(var_decl) };

  try
  {
    return generateVariableCreation(var_type, var_decl->name->getName(), constructValue(var_type, init));
  }
  catch (const TooManyArgumentInInitialization & e)
  {
    throw TooManyArgumentInVariableInitialization{ e.pos };
  }
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::BraceInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ dpos(var_decl) };

  try
  {
    return generateVariableCreation(var_type, var_decl->name->getName(), constructValue(var_type, init));
  }
  catch (const TooManyArgumentInInitialization & e)
  {
    throw TooManyArgumentInVariableInitialization{ e.pos };
  }
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & input_var_type, const std::shared_ptr<ast::AssignmentInitialization> & init)
{
  auto value = generateExpression(init->value);

  Type var_type = input_var_type;
  if (input_var_type.baseType() == Type::Auto)
  {
    var_type = value->type();
    if (var_type.isConst())
      var_type = var_type.withFlag(Type::ConstFlag);
    else if (var_type.isReference())
      var_type = var_type.withFlag(Type::ReferenceFlag);
  }

  ConversionSequence seq = ConversionSequence::compute(value, var_type, engine());
  if (seq == ConversionSequence::NotConvertible())
    throw CouldNotConvert{ dpos(init), dstr(value->type()), dstr(var_type) };
  
  /// TODO : this is not optimal I believe
  // we could add copy elision
  value = prepareFunctionArgument(value, var_type, seq);
  return generateVariableCreation(var_type, var_decl->name->getName(), value);
}



std::shared_ptr<program::Statement> FunctionCompiler::generateFundamentalVariableCreation(const Type & type, const std::string & name)
{
  return generateVariableCreation(type, name, constructFundamentalValue(type, true));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableCreation(const Type & type, const std::string & name, const std::shared_ptr<program::Expression> & value)
{
  const int stack_index = std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(name, type);

  auto var_decl = program::PushValue::New(type, name, value, stack_index);

  if (std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->category() == FunctionScope::FunctionBody && isCompilingAnonymousFunction())
  {
    mStack[stack_index].global = true;
    std::vector<std::shared_ptr<program::Statement>> statements;
    statements.push_back(var_decl);
    statements.push_back(program::PushGlobal::New(stack_index));
    
    auto simpl = mScript.implementation();
    simpl->register_global(Type::ref(mStack[stack_index].type), mStack[stack_index].name);

    return program::CompoundStatement::New(std::move(statements));
  }

  return var_decl;
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDestruction(const Variable & var)
{
  if(var.global)
    return program::PopValue::New(false, Function{}, var.index);

  if (var.type.isObjectType())
  {
    Function dtor = engine()->getClass(var.type).destructor();
    if (dtor.isNull())
      throw ObjectHasNoDestructor{};
  
    return program::PopValue::New(true, dtor, var.index);
  }

  return program::PopValue::New(true, Function{}, var.index);
}

std::shared_ptr<program::Statement> FunctionCompiler::generateWhileLoop(const std::shared_ptr<ast::WhileLoop> & whileLoop)
{
  auto cond = generateExpression(whileLoop->condition);

  /// TODO : convert to bool if not bool
  if (cond->type().baseType() != Type::Boolean)
    throw NotImplementedError{ "FunctionCompiler::generateWhileLoop() : implicit conversion to bool not implemented" };

  std::shared_ptr<program::Statement> body;
  if (whileLoop->body->is<ast::CompoundStatement>())
    body = generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(body), FunctionScope::WhileBody);
  else
    body = generateStatement(whileLoop->body);

  return program::WhileLoop::New(cond, body);
}

} // namespace compiler

} // namespace script

