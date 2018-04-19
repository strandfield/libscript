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

  return ExtensibleScope::lookup(name, nl);
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

  EnterScope guard{ this, FunctionScope::FunctionArguments };

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

void FunctionCompiler::enter_scope(FunctionScope::Category scopeType, const ScopeKey &)
{
  mCurrentScope = Scope{ std::make_shared<FunctionScope>(this, scopeType, mCurrentScope) };
  if (scopeType == FunctionScope::FunctionBody)
    mFunctionBodyScope = mCurrentScope;
  else if (scopeType == FunctionScope::FunctionArguments)
    mFunctionArgumentsScope = mCurrentScope;
}

void FunctionCompiler::leave_scope(const ScopeKey &)
{
  std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->destroy();
  mCurrentScope = mCurrentScope.parent();
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
  case ast::NodeType::NamespaceAliasDef:
    processNamespaceAlias(std::static_pointer_cast<ast::NamespaceAliasDefinition>(s));
    return;
  case ast::NodeType::TypeAliasDecl:
    processTypeAlias(std::static_pointer_cast<ast::TypeAliasDeclaration>(s));
    return;
  case ast::NodeType::UsingDeclaration:
    processUsingDeclaration(std::static_pointer_cast<ast::UsingDeclaration>(s));
    return;
  case ast::NodeType::UsingDirective:
    processUsingDirective(std::static_pointer_cast<ast::UsingDirective>(s));
    return;
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

  // destroying default arguments
  if (fscp->category() == FunctionScope::FunctionBody)
  {
    const int default_count = mFunction.prototype().defaultArgCount();
    for (int i(mFunction.prototype().argc() - 1); i >= mFunction.prototype().argc() - default_count; --i)
    {
      const Type & arg_type = mFunction.prototype().argv(i);
      const bool destroy = !(arg_type.isReference() || arg_type.isRefRef());
      write(program::PopDefaultArgument::New(i, destroy));
    }
  }
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
  auto expr = generateExpression(es->expression);
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
    for_cond = generateExpression(fl->condition);

  if (for_cond->type().baseType() != Type::Boolean)
  {
    /// TODO : simply perform an implicit conversion
    throw NotImplementedError{ "Implicit conversion to bool not implemented yet in for-condition" };
  }

  std::shared_ptr<program::Expression> for_loop_incr = nullptr;
  if (fl->loopIncrement == nullptr)
    for_loop_incr = program::Literal::New(engine()->newBool(true)); /// TODO : what should we do ? (perhaps use a statement instead, an use the null statement)
  else
    for_loop_incr = generateExpression(fl->loopIncrement);

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
  auto cond = generateExpression(is->condition);
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

  Module m = engine()->getModule(id->at(0));
  if (m.isNull())
    throw UnknownModuleName{ dpos(id), id->at(0) };

  for (size_t i(1); i < id->size(); ++i)
  {
    Module child = m.getSubModule(id->at(i));
    if (child.isNull())
      throw UnknownSubModuleName{ dpos(id), id->at(i), m.name() };

    m = child;
  }

  m.load();

  mCurrentScope.merge(m.scope());
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

void FunctionCompiler::processNamespaceAlias(const std::shared_ptr<ast::NamespaceAliasDefinition> & decl)
{
  /// TODO : merge this duplicate of ScriptCompiler

  /// TODO : check that alias_name is a simple identifier or enforce it in the parser
  const std::string & name = decl->alias_name->getName();

  std::vector<std::string> nested;
  auto target = decl->aliased_namespace;
  while (target->is<ast::ScopedIdentifier>())
  {
    const auto & scpid = target->as<ast::ScopedIdentifier>();
    nested.push_back(scpid.rhs->getName()); /// TODO : check that all names are simple ids
    target = scpid.lhs;
  }
  nested.push_back(target->getName());

  std::reverse(nested.begin(), nested.end());
  NamespaceAlias alias{ name, std::move(nested) };

  mCurrentScope.inject(alias); /// TODO : this may throw and we should handle that
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

  auto retval = generateExpression(rs->expression);

  const ConversionSequence conv = ConversionSequence::compute(retval, mFunction.prototype().returnType(), engine());

  /// TODO : write a dedicated function for this, don't use prepareFunctionArg()
  retval = prepareFunctionArgument(retval, mFunction.prototype().returnType(), conv);

  write(program::ReturnStatement::New(retval, std::move(statements)));
}

void FunctionCompiler::processTypeAlias(const std::shared_ptr<ast::TypeAliasDeclaration> & decl)
{
  /// TODO : merge this duplicate of ScriptCompiler

  /// TODO : check that alias_name is a simple identifier or enforce it in the parser
  const std::string & name = decl->alias_name->getName();

  NameLookup lookup = resolve(decl->aliased_type);
  if (lookup.typeResult().isNull())
    throw InvalidTypeName{ dpos(decl), dstr(decl->aliased_type) };

  mCurrentScope.inject(name, lookup.typeResult());
}

void FunctionCompiler::processUsingDeclaration(const std::shared_ptr<ast::UsingDeclaration> & decl)
{
  /// TODO : merge this duplicate of ScriptCompiler

  NameLookup lookup = resolve(decl->used_name);
  /// TODO : throw exception if nothing found
  mCurrentScope.inject(lookup.impl().get());
}

void FunctionCompiler::processUsingDirective(const std::shared_ptr<ast::UsingDirective> & decl)
{
  /// TODO : merge this duplicate of ScriptCompiler
  throw NotImplementedError{ dpos(decl), "Using directive not implemented yet" };
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl)
{
  const Type var_type = resolve(var_decl->variable_type);

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
    processVariableCreation(var_type, var_decl->name->getName(), constructValue(var_type, nullptr, dpos(var_decl)));
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
    processVariableCreation(var_type, var_decl->name->getName(), constructValue(var_type, init));
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
    processVariableCreation(var_type, var_decl->name->getName(), constructValue(var_type, init));
  }
  catch (const TooManyArgumentInInitialization & e)
  {
    throw TooManyArgumentInVariableInitialization{ e.pos };
  }
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & input_var_type, const std::shared_ptr<ast::AssignmentInitialization> & init)
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
  processVariableCreation(var_type, var_decl->name->getName(), value);
}



void FunctionCompiler::processFundamentalVariableCreation(const Type & type, const std::string & name)
{
  processVariableCreation(type, name, constructFundamentalValue(type, true));
}

void FunctionCompiler::processVariableCreation(const Type & type, const std::string & name, const std::shared_ptr<program::Expression> & value)
{
  const int stack_index = std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(name, type);

  write(program::PushValue::New(type, name, value, stack_index));

  if (std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->category() == FunctionScope::FunctionBody && isCompilingAnonymousFunction())
  {
    mStack[stack_index].global = true;

    auto simpl = mScript.implementation();
    simpl->register_global(Type::ref(mStack[stack_index].type), mStack[stack_index].name);

    write(program::PushGlobal::New(stack_index));
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
  auto cond = generateExpression(whileLoop->condition);

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

