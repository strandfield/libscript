// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/functioncompiler.h"
#include "script/private/functionscope_p.h"

#include "script/compiler/compilererrors.h"

#include "script/compiler/assignmentcompiler.h"
#include "script/compiler/constructorcompiler.h"
#include "script/compiler/destructorcompiler.h"
#include "script/compiler/lambdacompiler.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/valueconstructor.h"

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/cast.h"
#include "script/private/function_p.h"
#include "script/functiontype.h"
#include "script/lambda.h"
#include "script/literals.h"
#include "script/namelookup.h"
#include "script/private/namelookup_p.h"
#include "script/overloadresolution.h"
#include "script/private/script_p.h"

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
  , mSize(other.mSize)
  , mSp(other.mSp)
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

bool FunctionScope::lookup(const std::string & name, NameLookupImpl *nl) const
{
  /// TODO : this block is incorrect, 'this' is not always at index 1 !
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

  return ExtensibleScope::lookup(name, nl);
}

int FunctionScope::add_var(const std::string & name, const Type & t)
{
  int stack_index = mCompiler->mStack.addVar(t, name);
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

bool FunctionScope::catch_continue() const
{
  return mCategory == ForBody || mCategory == WhileBody;
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

Stack::Stack(int s) : size(0), capacity(s)
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
}

int Stack::addVar(const Type & t, const std::string & name)
{
  if (this->size == this->capacity)
  {
    this->realloc((this->size + 1) * 2);
  }

  this->data[this->size] = Variable{ t, name, this->size };
  this->size += 1;
  return this->size - 1;
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

const Variable & Stack::operator[](int i) const
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


EnterScope::EnterScope(FunctionCompiler *c, FunctionScope::Category scp)
  : compiler(c)
{
  assert(c != nullptr);
  c->enter_scope(scp, FunctionCompiler::ScopeKey{});
}

EnterScope::~EnterScope()
{
  leave();
}

void EnterScope::leave()
{
  if (compiler == nullptr)
    return;

  compiler->leave_scope(FunctionCompiler::ScopeKey{});
  compiler = nullptr;
}

StackVariableAccessor::StackVariableAccessor(Stack & s, FunctionCompiler* fc)
  : stack_(&s)
  , fcomp_(fc)
{

}

std::shared_ptr<program::Expression> StackVariableAccessor::global_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  Script s = script_;
  auto simpl = s.impl();
  const Type & gtype = simpl->global_types[offset];

  return program::FetchGlobal::New(s.id(), offset, gtype);
}

std::shared_ptr<program::Expression> StackVariableAccessor::local_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  const Type t = stack()[offset].type;
  return program::StackValue::New(offset, t);
}

FunctionCompilerLambdaProcessor::FunctionCompilerLambdaProcessor(Stack & s, FunctionCompiler* fc)
  : stack_(&s)
  , fcomp_(fc)
{

}

std::shared_ptr<program::LambdaExpression> FunctionCompilerLambdaProcessor::generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le)
{
  CompileLambdaTask task;
  task.lexpr = le;
  task.scope = ec.scope();

  const int first_capturable = ec.caller().isDestructor() || ec.caller().isConstructor() ? 0 : 1;
  LambdaCompiler::preprocess(task, &ec, stack(), first_capturable);

  LambdaCompiler compiler{ fcomp_->session() };
  LambdaCompilationResult result = compiler.compile(task);

  return result.expression;
}


Engine* FunctionCompilerModuleLoader::engine() const
{
  return compiler_->engine();
}

Script FunctionCompilerModuleLoader::load(const SourceFile &src)
{
  Script s = engine()->newScript(src);
  bool success = engine()->compile(s);
  if (!success)
  {
    std::string mssg;
    for (const auto & m : s.messages())
      mssg += m.to_string() + "\n";
    throw ModuleImportationError{ src.filepath(), mssg };
  }

  s.run();

  return s;
}


FunctionCompiler::FunctionCompiler(const std::shared_ptr<CompileSession> & s)
  : Compiler(s)
  , variable_(mStack, this)
  , lambda_(mStack, this)
{
  expr_.setVariableAccessor(variable_);
  expr_.setLambdaProcessor(lambda_);
  scope_statements_.scope_ = &mCurrentScope;
  modules_.loader_.compiler_ = this;
}


void FunctionCompiler::compile(const CompileFunctionTask & task)
{
  mScript = task.function.script();

  variable_.script() = mScript;
  lambda_.script() = mScript;
  expr_.setCaller(task.function);
  
  mFunction = task.function;
  mDeclaration = task.declaration;
  mBaseScope = task.scope;
  mCurrentScope = task.scope;

  mStack.clear();

  const Prototype & proto = mFunction.prototype();
  if(!mFunction.isDestructor())
    mStack.addVar(proto.returnType(), "return-value");

  EnterScope guard{ this, FunctionScope::FunctionArguments };

  for (int i(0); i < proto.count(); ++i)
    std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(argumentName(i), proto.at(i));

  std::shared_ptr<program::CompoundStatement> body = generateBody();
  /// TODO : add implicit return statement in void functions
  mFunction.impl()->set_impl(body);
}


Script FunctionCompiler::script()
{
  return mScript;
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
  return dynamic_cast<const ScriptFunctionImpl*>(compiledFunction().impl().get()) != nullptr;
}

std::string FunctionCompiler::argumentName(int index)
{
  auto funcdecl = std::dynamic_pointer_cast<ast::FunctionDecl>(declaration());

  if (mFunction.hasImplicitObject())
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


bool FunctionCompiler::canUseThis() const
{
  return mFunction.hasImplicitObject() || mFunction.isConstructor() || mFunction.isDestructor();
}

std::shared_ptr<program::Expression> FunctionCompiler::generate(const std::shared_ptr<ast::Expression> & e)
{
  expr_.setScope(mCurrentScope);
  return expr_.generateExpression(e);
}

void FunctionCompiler::enter_scope(FunctionScope::Category scopeType, const ScopeKey &)
{
  mCurrentScope = Scope{ std::make_shared<FunctionScope>(this, scopeType, mCurrentScope) };
  if (scopeType == FunctionScope::FunctionBody)
    mFunctionBodyScope = mCurrentScope;
  else if (scopeType == FunctionScope::FunctionArguments)
    mFunctionArgumentsScope = mCurrentScope;

  expr_.setScope(mCurrentScope);
}

void FunctionCompiler::leave_scope(const ScopeKey &)
{
  std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->destroy();
  mCurrentScope = mCurrentScope.parent();
}

NameLookup FunctionCompiler::resolve(const std::shared_ptr<ast::Identifier> & name)
{
  return NameLookup::resolve(name, mCurrentScope);
}

Scope FunctionCompiler::breakScope() const
{
  Scope s = mCurrentScope;
  while (!std::dynamic_pointer_cast<FunctionScope>(s.impl())->catch_break())
    s = s.parent();
  return s;
}

Scope FunctionCompiler::continueScope() const
{
  Scope s = mCurrentScope;
  while (!std::dynamic_pointer_cast<FunctionScope>(s.impl())->catch_continue())
    s = s.parent();
  return s;
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateBody()
{
  if (!mFunction.isDefaulted())
  {
    auto body = this->generateCompoundStatement(bodyDeclaration(), FunctionScope::FunctionBody);

    if (mFunction.isConstructor())
    {
      EnterScope guard{ this, FunctionScope::FunctionBody };

      auto constructor_header = generateConstructorHeader();
      body->statements.insert(body->statements.begin(), constructor_header->statements.begin(), constructor_header->statements.end());
    }
    else if (mFunction.isDestructor())
    {
      EnterScope guard{ this, FunctionScope::FunctionBody };

      auto destructor_footer = generateDestructorFooter();
      body->statements.insert(body->statements.end(), destructor_footer->statements.begin(), destructor_footer->statements.end());
    }
    else if (isCompilingAnonymousFunction())
    {
      EnterScope guard{ this, FunctionScope::FunctionBody };
      /// TODO : should we generate a dummy program::ReturnStatement ?
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
      && op.prototype().at(0) == Type::ref(op.memberOf().id())
      && op.prototype().at(1) == Type::cref(op.memberOf().id()))
      return AssignmentCompiler{ this }.generateAssignmentOperator();
  }

  throw FunctionCannotBeDefaulted{ dpos(mDeclaration) };
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


FunctionCompiler::BufferSwap::BufferSwap(buffer_type & a, buffer_type & b)
{
  first = &a;
  second = &b;
  std::swap(a, b);
}

FunctionCompiler::BufferSwap::~BufferSwap()
{
  std::swap(*first, *second);
}

void FunctionCompiler::write(const std::shared_ptr<program::Statement> & s)
{
  mBuffer.push_back(s);
}

size_t FunctionCompiler::buffer_size()
{
  return mBuffer.size();
}

std::vector<std::shared_ptr<program::Statement>> FunctionCompiler::resize_buffer(size_t size)
{
  std::vector<std::shared_ptr<program::Statement>> result{ mBuffer.begin() + size, mBuffer.end() };
  mBuffer.resize(size);
  return result;
}

std::vector<std::shared_ptr<program::Statement>> FunctionCompiler::read(size_t count)
{
  std::vector<std::shared_ptr<program::Statement>> result{ mBuffer.end() - count, mBuffer.end() };
  mBuffer.resize(mBuffer.size() - count);
  return result;
}

std::shared_ptr<program::Statement> FunctionCompiler::read_one()
{
  auto ret = mBuffer.back();
  mBuffer.pop_back();
  return ret;
}

std::shared_ptr<program::Statement> FunctionCompiler::generate(const std::shared_ptr<ast::Statement> & s)
{
  const auto size = buffer_size();

  process(s);

  if (buffer_size() == size)
    return program::CompoundStatement::New(); // simulates a null statement
  else if (buffer_size() == size + 1)
    return read_one();

  return program::CompoundStatement::New(resize_buffer(size));
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & cs, FunctionScope::Category scopeType)
{
  EnterScope guard{ this, scopeType };

  const auto size = buffer_size();

  for (const auto & s : cs->statements)
    process(s);

  processExitScope(mCurrentScope);

  return program::CompoundStatement::New(resize_buffer(size));
}

void FunctionCompiler::process(const std::shared_ptr<ast::Statement> & s)
{
  switch (s->type())
  {
  case ast::NodeType::NullStatement:
    return;
  case ast::NodeType::IfStatement:
    processIfStatement(std::dynamic_pointer_cast<ast::IfStatement>(s));
    return;
  case ast::NodeType::WhileLoop:
    processWhileLoop(std::dynamic_pointer_cast<ast::WhileLoop>(s));
    return;
  case ast::NodeType::ForLoop:
    processForLoop(std::dynamic_pointer_cast<ast::ForLoop>(s));
    return;
  case ast::NodeType::CompoundStatement:
    processCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(s), FunctionScope::CompoundStatement);
    return;
  case ast::NodeType::VariableDeclaration:
    processVariableDeclaration(std::dynamic_pointer_cast<ast::VariableDecl>(s));
    return;
  case ast::NodeType::BreakStatement:
  case ast::NodeType::ContinueStatement:
  case ast::NodeType::ReturnStatement:
    processJumpStatement(std::dynamic_pointer_cast<ast::JumpStatement>(s));
    return;
  case ast::NodeType::ExpressionStatement:
    processExpressionStatement(std::dynamic_pointer_cast<ast::ExpressionStatement>(s));
    return;
  case ast::NodeType::UsingDirective:
  case ast::NodeType::UsingDeclaration:
  case ast::NodeType::NamespaceAliasDef:
  case ast::NodeType::TypeAliasDecl:
    return scope_statements_.process(s);
  case ast::NodeType::ImportDirective:
    processImportDirective(std::static_pointer_cast<ast::ImportDirective>(s));
    return;
  default:
    break;
  }

  assert(false);
  throw std::runtime_error{ "FunctionCompiler::process() : not implemented" };
}

void FunctionCompiler::processExitScope(const Scope & scp)
{
  assert(dynamic_cast<const FunctionScope *>(scp.impl().get()) != nullptr);

  const FunctionScope *fscp = dynamic_cast<const FunctionScope *>(scp.impl().get());

  const int sp = fscp->sp();
  Stack & stack = mStack;

  for (int i(stack.size - 1); i >= sp; --i)
    processVariableDestruction(stack.at(i));
}

void FunctionCompiler::generateExitScope(const Scope & scp, std::vector<std::shared_ptr<program::Statement>> & statements)
{
  BufferSwap swap{ mBuffer, statements };
  processExitScope(scp);
}

void FunctionCompiler::processCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & cs, FunctionScope::Category scopeType)
{
  EnterScope guard{ this, scopeType };

  for (const auto & s : cs->statements)
    process(s);

  processExitScope(mCurrentScope);
}

void FunctionCompiler::processExpressionStatement(const std::shared_ptr<ast::ExpressionStatement> & es)
{
  auto expr = generate(es->expression);
  write(program::ExpressionStatement::New(expr));
}

void FunctionCompiler::processForLoop(const std::shared_ptr<ast::ForLoop> & fl)
{
  EnterScope guard{ this, FunctionScope::ForInit };

  std::shared_ptr<program::Statement> for_init = nullptr;
  if (fl->initStatement != nullptr)
    for_init = generate(fl->initStatement);

  std::shared_ptr<program::Expression> for_cond = nullptr;
  if (fl->condition == nullptr)
    for_cond = program::Literal::New(engine()->newBool(true));
  else
    for_cond = generate(fl->condition);

  if (for_cond->type().baseType() != Type::Boolean)
  {
    /// TODO : simply perform an implicit conversion
    throw NotImplementedError{ "Implicit conversion to bool not implemented yet in for-condition" };
  }

  std::shared_ptr<program::Expression> for_loop_incr = nullptr;
  if (fl->loopIncrement == nullptr)
    for_loop_incr = program::Literal::New(engine()->newBool(true)); /// TODO : what should we do ? (perhaps use a statement instead, an use the null statement)
  else
    for_loop_incr = generate(fl->loopIncrement);

  std::shared_ptr<program::Statement> body = nullptr;
  if (fl->body->is<ast::CompoundStatement>())
    body = this->generateCompoundStatement(std::static_pointer_cast<ast::CompoundStatement>(fl->body), FunctionScope::ForBody);
  else
    body = generate(fl->body);


  std::vector<std::shared_ptr<program::Statement>> statements;
  generateExitScope(mCurrentScope, statements);

  write(program::ForLoop::New(for_init, for_cond, for_loop_incr, body, program::CompoundStatement::New(std::move(statements))));
}

void FunctionCompiler::processIfStatement(const std::shared_ptr<ast::IfStatement> & is)
{
  auto cond = generate(is->condition);
  std::shared_ptr<program::Statement> body;
  if (is->body->is<ast::CompoundStatement>())
    body = generateCompoundStatement(std::static_pointer_cast<ast::CompoundStatement>(is->body), FunctionScope::IfBody);
  else
    body = generate(is->body);

  auto result = program::IfStatement::New(cond, body);

  if (is->elseClause != nullptr)
    result->elseClause = generate(is->elseClause);

  write(result);
}

void FunctionCompiler::processImportDirective(const std::shared_ptr<ast::ImportDirective> & id)
{
  if (id->export_keyword.isValid())
  {
    log(diagnostic::error() << dpos(id->export_keyword) << "'export' are only allowed at script level");
  }

  Scope imported = modules_.process(id);
  mCurrentScope.merge(imported);
}

void FunctionCompiler::processJumpStatement(const std::shared_ptr<ast::JumpStatement> & js)
{
  if (js->is<ast::ReturnStatement>())
    return processReturnStatement(std::dynamic_pointer_cast<ast::ReturnStatement>(js));

  std::vector<std::shared_ptr<program::Statement>> statements;

  if (js->is<ast::BreakStatement>())
  {
    const Scope scp = breakScope();
    generateExitScope(scp, statements);
    write(program::BreakStatement::New(std::move(statements)));
    return;
  }
  else if (js->is<ast::ContinueStatement>())
  {
    const Scope scp = continueScope();
    generateExitScope(scp, statements);
    write(program::ContinueStatement::New(std::move(statements)));
    return;
  }

  assert(false);
  throw NotImplementedError{ dpos(js), "This kind of jump statement not implemented" };
}

void FunctionCompiler::processReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  generateExitScope(mFunctionBodyScope, statements);

  if (rs->expression == nullptr)
  {
    if (mFunction.prototype().returnType() != Type::Void)
      throw ReturnStatementWithoutValue{};

    return write(program::ReturnStatement::New(nullptr, std::move(statements)));
  }
  else
  {
    if (mFunction.prototype().returnType() == Type::Void)
      throw ReturnStatementWithValue{};
  }

  auto retval = generate(rs->expression);

  const ConversionSequence conv = ConversionSequence::compute(retval, mFunction.prototype().returnType(), engine());

  /// TODO : write a dedicated function for this, don't use prepareFunctionArg()
  retval = ConversionProcessor::convert(engine(), retval, mFunction.prototype().returnType(), conv);

  write(program::ReturnStatement::New(retval, std::move(statements)));
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl)
{
  const Type var_type = type_.resolve(var_decl->variable_type, mCurrentScope);

  if (var_decl->init == nullptr)
    return processVariableDeclaration(var_decl, var_type, nullptr);

  assert(var_decl->init != nullptr);

  if (var_decl->init->is<ast::AssignmentInitialization>())
    return processVariableDeclaration(var_decl, var_type, std::dynamic_pointer_cast<ast::AssignmentInitialization>(var_decl->init));
  else if (var_decl->init->is<ast::ConstructorInitialization>())
    return processVariableDeclaration(var_decl, var_type, std::dynamic_pointer_cast<ast::ConstructorInitialization>(var_decl->init));
  else
    return processVariableDeclaration(var_decl, var_type, std::dynamic_pointer_cast<ast::BraceInitialization>(var_decl->init));
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, std::nullptr_t)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ dpos(var_decl) };

  try
  {
    expr_.setScope(mCurrentScope);
    processVariableCreation(var_type, var_decl->name->getName(), ValueConstructor::construct(engine(), var_type, nullptr, dpos(var_decl)));
  }
  catch (const EnumerationsCannotBeDefaultConstructed & e)
  {
    throw EnumerationsMustBeInitialized{ e.pos };
  }
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::ConstructorInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ dpos(var_decl) };

  try
  {
    expr_.setScope(mCurrentScope);
    processVariableCreation(var_type, var_decl->name->getName(), ValueConstructor::construct(expr_, var_type, init));
  }
  catch (const TooManyArgumentInInitialization & e)
  {
    throw TooManyArgumentInVariableInitialization{ e.pos };
  }
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::BraceInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ dpos(var_decl) };

  try
  {
    expr_.setScope(mCurrentScope);
    processVariableCreation(var_type, var_decl->name->getName(), ValueConstructor::construct(expr_, var_type, init));
  }
  catch (const TooManyArgumentInInitialization & e)
  {
    throw TooManyArgumentInVariableInitialization{ e.pos };
  }
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & input_var_type, const std::shared_ptr<ast::AssignmentInitialization> & init)
{
  auto value = generate(init->value);

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
    throw CouldNotConvert{ dpos(init), value->type(), var_type };

  /// TODO : this is not optimal I believe
  // we could add copy elision
  value = ConversionProcessor::convert(engine(), value, var_type, seq);
  processVariableCreation(var_type, var_decl->name->getName(), value);
}

void FunctionCompiler::processVariableCreation(const Type & type, const std::string & name, const std::shared_ptr<program::Expression> & value)
{
  const int stack_index = std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(name, type);

  write(program::PushValue::New(type, name, value, stack_index));

  if (std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->category() == FunctionScope::FunctionBody && isCompilingAnonymousFunction())
  {
    mStack[stack_index].global = true;

    auto simpl = mScript.impl();
    simpl->register_global(Type::ref(mStack[stack_index].type), mStack[stack_index].name);

    write(program::PushGlobal::New(script().id(), stack_index));
  }
}

void FunctionCompiler::processVariableDestruction(const Variable & var)
{
  if (var.global)
    return write(program::PopValue::New(false, Function{}, var.index));

  if (var.type.isObjectType())
  {
    Function dtor = engine()->getClass(var.type).destructor();
    if (dtor.isNull())
      throw ObjectHasNoDestructor{};

    return write(program::PopValue::New(true, dtor, var.index));
  }

  write(program::PopValue::New(true, Function{}, var.index));
}

void FunctionCompiler::processWhileLoop(const std::shared_ptr<ast::WhileLoop> & whileLoop)
{
  auto cond = generate(whileLoop->condition);

  /// TODO : convert to bool if not bool
  if (cond->type().baseType() != Type::Boolean)
    throw NotImplementedError{ "FunctionCompiler::generateWhileLoop() : implicit conversion to bool not implemented" };

  std::shared_ptr<program::Statement> body;
  if (whileLoop->body->is<ast::CompoundStatement>())
    body = generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(body), FunctionScope::WhileBody);
  else
    body = generate(whileLoop->body);

  write(program::WhileLoop::New(cond, body));
}

} // namespace compiler

} // namespace script

