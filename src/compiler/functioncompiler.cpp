// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/functioncompiler.h"

#include "script/compiler/compilererrors.h"

#include "script/compiler/lambdacompiler.h"

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/cast.h"
#include "../function_p.h"
#include "script/functiontype.h"
#include "script/lambda.h"
#include "script/namelookup.h"
#include "../namelookup_p.h"
#include "script/overloadresolution.h"
#include "../script_p.h"

namespace script
{

namespace compiler
{

compiler::FunctionScope::FunctionScope(FunctionCompiler *c, Type st)
  : compiler(c)
{
  enterScope(st);
}

FunctionScope::~FunctionScope()
{
  leaveScope();
}

int FunctionScope::sp() const
{
  return this->compiler->mScopeStack.at(mIndex).offset;
}

void FunctionScope::enterScope(Type scopeType)
{
  this->compiler->mScopeStack.push_back(ScopeData{ scopeType, this->compiler->mStack.size });
  mIndex = this->compiler->mScopeStack.size() - 1;
}

void FunctionScope::leaveScope()
{
  ScopeData scp = this->compiler->mScopeStack.back();
  this->compiler->mScopeStack.pop_back();
  this->compiler->mStack.destroy(compiler->mStack.size - scp.offset);
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
  mCurrentScope = task.scope;

  mScopeStack.clear();
  mStack.clear();

  const Prototype & proto = mFunction.prototype();
  if(!mFunction.isDestructor())
    mStack.addVar(proto.returnType(), "return-value");
  for (int i(0); i < proto.argc(); ++i)
    mStack.addVar(proto.argv(i), argumentName(i));

  std::shared_ptr<program::CompoundStatement> body = generateBody();

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

script::Scope FunctionCompiler::currentScope() const
{
  return mCurrentScope;
}

Class FunctionCompiler::classScope()
{
  return mCurrentScope.asClass();
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


void FunctionCompiler::enterScope(FunctionScope::Type scopeType)
{
  mScopeStack.push_back(ScopeData{ scopeType, mStack.size });
}

void FunctionCompiler::leaveScope(int depth)
{
  assert(depth <= static_cast<int>(mScopeStack.size()));

  for (int i(0); i < depth; ++i)
  {
    ScopeData scp = mScopeStack.back();
    mScopeStack.pop_back();
    mStack.destroy(mStack.size - scp.offset);
  }
}


int FunctionCompiler::breakDepth()
{
  // Returns the number of scopes that would be affected by a break statement.
  // This includes all scopes up to a 'while-body' or a 'for-init' scope.
  // Returns -1 if no scope are affected by the 'break'.

  const ScopeStack & scpStack = mScopeStack;

  int i(scpStack.size() - 1);
  int depth(1);
  while (i >= 0)
  {
    if (scpStack[i].type == FunctionScope::ForInit || scpStack[i].type == FunctionScope::WhileBody)
      return depth;

    ++depth;
    --i;
  }

  return -1;
}

int FunctionCompiler::continueDepth()
{
  return breakDepth();
}


NameLookup FunctionCompiler::unqualifiedLookup(const std::shared_ptr<ast::Identifier> & name)
{
  assert(name->type() == ast::NodeType::SimpleIdentifier);

  if (name->name == parser::Token::This)
  {
    if (!this->canUseThis())
      throw IllegalUseOfThis{ name->pos().line, name->pos().col };

    auto result = std::make_shared<NameLookupImpl>();
    result->localIndex = 1;
    return NameLookup{ result };
  }

  const Stack & stack = mStack;
  const std::string & str = name->getName();
  const int offset = stack.lastIndexOf(str);
  if (offset != -1)
  {
    auto result = std::make_shared<NameLookupImpl>();
    result->localIndex = offset;
    return NameLookup{ result };
  }

  return NameLookup::resolve(name, currentScope());
}

std::vector<Operator> FunctionCompiler::getOperators(Operator::BuiltInOperator op, Type type, int lookup_policy)
{
  std::vector<Operator> ret = AbstractExpressionCompiler::getOperators(op, type, lookup_policy);

  if (lookup_policy & OperatorLookupPolicy::ConsiderCurrentScope)
  {
    const auto & ops = getScopeOperators(op, currentScope(), OperatorLookupPolicy::FetchParentOperators);
    ret.insert(ret.end(), ops.begin(), ops.end());
  }

  if (lookup_policy & OperatorLookupPolicy::RemoveDuplicates)
    return removeDuplicates(ret);

  return ret;
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
    return generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(statement), compiler::FunctionScope::CompoundStatement);
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
      return generateAssignmentOperator();
  }

  throw FunctionCannotBeDefaulted{ mDeclaration->pos().line, mDeclaration->pos().col };
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

std::shared_ptr<program::Expression> FunctionCompiler::defaultConstructMember(const Type & t)
{
  assert(!t.isReference());

  if (t.isFundamentalType())
    return constructFundamentalValue(t, true);
  else if (t.isEnumType())
    throw EnumMemberCannotBeDefaultConstructed{};
  else if (t.isObjectType())
  {
    Function default_ctor = engine()->getClass(t).defaultConstructor();
    if (default_ctor.isNull())
      throw DataMemberHasNoDefaultConstructor{};
    else if (default_ctor.isDeleted())
      throw DataMemberHasDeletedDefaultConstructor{};

    return program::ConstructorCall::New(default_ctor, {});
  }

  throw NotImplementedError{ "FunctionCompiler::defaultConstructMember()" };
}

std::shared_ptr<program::Expression> FunctionCompiler::constructDataMember(const Type & t, std::vector<std::shared_ptr<program::Expression>> && args)
{
  assert(!t.isReference());

  /// TODO : add better diagnostic for this first two blocks
  if (t.isFundamentalType())
  {
    if (args.size() > 1)
      throw TooManyArgumentsInMemberInitialization{};

    ConversionSequence seq = ConversionSequence::compute(args.front(), t, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw NotImplementedError{"FunctionCompiler::constructDataMember() :  not convertible"  };

    auto value = prepareFunctionArgument(args.front(), t, seq);
    return value;
  }
  else if (t.isEnumType()) /// TODO : could this block be merged with the one above ?
  {
    if (args.size() > 1)
      throw TooManyArgumentsInMemberInitialization{};

    ConversionSequence seq = ConversionSequence::compute(args.front(), t, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw NotImplementedError{ "FunctionCompiler::constructDataMember() :  not convertible" };

    auto value = prepareFunctionArgument(args.front(), t, seq);
    return value;
  }
  else if (t.isFunctionType())
    throw NotImplementedError{ "FunctionCompiler::constructDataMember() : function data member" };

  const std::vector<Function> & ctors = engine()->getClass(t).constructors();
  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(ctors, args))
    throw CouldNotFindValidConstructor{ }; /// TODO add a better diagnostic message

  const Function ctor = resol.selectedOverload();
  const auto & conversions = resol.conversionSequence();

  prepareFunctionArguments(args, ctor.prototype(), conversions);
  return program::ConstructorCall::New(ctor, std::move(args));
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateConstructorHeader()
{
  /// TODO : refactor and disallow narrowing conversions when braceinitialization is used

  auto ctor_decl = std::dynamic_pointer_cast<ast::ConstructorDecl>(declaration());

  auto this_object = generateThisAccess();

  std::vector<ast::MemberInitialization> initializers = ctor_decl->memberInitializationList;

  std::shared_ptr<program::Statement> parent_ctor_call;
  for (size_t i(0); i < initializers.size(); ++i)
  {
    const auto & minit = initializers.at(i);
    NameLookup lookup = resolve(minit.name);
    if (lookup.typeResult() == classScope().id()) // delegating constructor
    {
      if (initializers.size() != 1)
        throw InvalidUseOfDelegatedConstructor{};

      std::vector<std::shared_ptr<program::Expression>> args;
      if (minit.init->is<ast::ConstructorInitialization>())
        args = generateExpressions(minit.init->as<ast::ConstructorInitialization>().args);
      else
        args = generateExpressions(minit.init->as<ast::BraceInitialization>().args);
      return program::CompoundStatement::New({ generateDelegateConstructorCall(args) });
    }
    else if (!classScope().parent().isNull() && lookup.typeResult() == classScope().parent().id()) // parent constructor call
    {
      std::vector<std::shared_ptr<program::Expression>> args;
      if (minit.init->is<ast::ConstructorInitialization>())
        args = generateExpressions(minit.init->as<ast::ConstructorInitialization>().args);
      else
        args = generateExpressions(minit.init->as<ast::BraceInitialization>().args);
      parent_ctor_call = generateParentConstructorCall(args);

      // removes m-initializer from list
      std::swap(initializers.back(), initializers.at(i));
      initializers.pop_back();
      break;
    }
  }

  if (parent_ctor_call == nullptr && !classScope().parent().isNull())
  {
    std::vector<std::shared_ptr<program::Expression>> args;
    parent_ctor_call = generateParentConstructorCall(args);
  }

  // Initializating data members
  const auto & data_members = classScope().dataMembers();
  const int data_members_offset = classScope().attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (const auto & minit : initializers)
  {
    NameLookup lookup = resolve(minit.name);
    if (lookup.resultType() != NameLookup::DataMemberName)
      throw NotDataMember{};

    if (lookup.dataMemberIndex() - data_members_offset < 0)
      throw InheritedDataMember{};

    assert(lookup.dataMemberIndex() - data_members_offset < static_cast<int>(members_initialization.size()));

    const int index = lookup.dataMemberIndex() - data_members_offset;
    if (members_initialization.at(index) != nullptr)
      throw DataMemberAlreadyHasInitializer{};

    std::vector<std::shared_ptr<program::Expression>> args;
    if (minit.init->is<ast::ConstructorInitialization>())
      args = generateExpressions(minit.init->as<ast::ConstructorInitialization>().args);
    else
      args = generateExpressions(minit.init->as<ast::BraceInitialization>().args);

    std::shared_ptr<program::Expression> member_value;
    const auto & dm = data_members.at(index);
    if (dm.type.isReference())
    {
      if (args.size() != 1)
        throw InvalidArgumentCountInDataMemberRefInit{ minit.name->pos().line, minit.name->pos().col };

      auto value = args.front();
      if (value->type().isConst() && !dm.type.isConst())
        throw CannotInitializeNonConstRefDataMemberWithConst{};

      if (!engine()->canCast(value->type(), dm.type))
        throw BadDataMemberRefInit{};

      member_value = value;
    }
    else
    {
      if (args.empty())
        member_value = defaultConstructMember(dm.type);
      else
        member_value = constructDataMember(dm.type, std::move(args));
    }


    members_initialization[index] = program::PushDataMember::New(member_value);
  }

  for (size_t i(0); i < members_initialization.size(); ++i)
  {
    if (members_initialization[i] != nullptr)
      continue;

    const auto & dm = data_members.at(i);

    std::shared_ptr<program::Expression> default_constructed_value = defaultConstructMember(dm.type);
    members_initialization[i] = program::PushDataMember::New(default_constructed_value);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  auto init_object = program::InitObjectStatement::New(classScope().id());
  statements.push_back(init_object);
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateDelegateConstructorCall(std::vector<std::shared_ptr<program::Expression>> & args)
{
  const std::vector<Function> & ctors = classScope().constructors();
  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(ctors, args))
    throw NoDelegatingConstructorFound{};

  auto object = program::StackValue::New(0, Type::ref(classScope().id()));
  Function ctor = resol.selectedOverload();
  const auto & convs = resol.conversionSequence();
  prepareFunctionArguments(args, ctor.prototype(), convs);
  return program::PlacementStatement::New(object, ctor, std::move(args));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateParentConstructorCall(std::vector<std::shared_ptr<program::Expression>> & args)
{
  const std::vector<Function> & ctors = classScope().parent().constructors();
  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(ctors, args))
    throw CouldNotFindValidBaseConstructor{};

  auto object = program::StackValue::New(0, Type::ref(classScope().id()));
  Function ctor = resol.selectedOverload();
  const auto & convs = resol.conversionSequence();
  prepareFunctionArguments(args, ctor.prototype(), convs);
  return program::PlacementStatement::New(object, ctor, std::move(args));
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateDestructorFooter()
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  const auto & data_members = classScope().dataMembers();
  for (int i(data_members.size() - 1); i >= 0; --i)
    statements.push_back(program::PopDataMember::New(getDestructor(data_members.at(i).type)));

  if (!classScope().parent().isNull())
  {
    /// TODO : check if dtor exists and is not deleted
    auto this_object = generateThisAccess();
    std::vector<std::shared_ptr<program::Expression>> args{ this_object };
    auto dtor_call = program::FunctionCall::New(classScope().parent().destructor(), std::move(args));
    statements.push_back(program::ExpressionStatement::New(dtor_call));
  }

  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateDefaultConstructor()
{
  return generateConstructorHeader();
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateCopyConstructor()
{
  auto this_object = generateThisAccess();
  auto other_object = program::StackValue::New(1, mStack[1].type);

  std::shared_ptr<program::Statement> parent_ctor_call;
  if (!classScope().parent().isNull())
  {
    Function parent_copy_ctor = classScope().parent().copyConstructor();
    if (parent_copy_ctor.isNull())
      throw ParentHasNoCopyConstructor{};
    else if (parent_copy_ctor.isDeleted())
      throw ParentHasDeletedCopyConstructor{};

    parent_ctor_call = program::PlacementStatement::New(this_object, parent_copy_ctor, { other_object });
  }

  // Initializating data members
  const auto & data_members = classScope().dataMembers();
  const int data_members_offset = classScope().attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);

    const std::shared_ptr<program::Expression> member_access = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);

    const ConversionSequence conv = ConversionSequence::compute(member_access, dm.type, engine());
    if (conv == ConversionSequence::NotConvertible())
      throw DataMemberIsNotCopyable{};

    members_initialization[i] = program::PushDataMember::New(prepareFunctionArgument(member_access, dm.type, conv));
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  else
    statements.push_back(program::InitObjectStatement::New(classScope().id()));
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateMoveConstructor()
{
  auto this_object = generateThisAccess();
  auto other_object = program::StackValue::New(1, mStack[1].type);

  std::shared_ptr<program::Statement> parent_ctor_call;
  if (!classScope().parent().isNull())
  {
    Function parent_move_ctor = classScope().parent().moveConstructor();
    if (!parent_move_ctor.isNull())
    {
      if (parent_move_ctor.isDeleted())
        throw ParentHasDeletedMoveConstructor{};
      parent_ctor_call = program::PlacementStatement::New(this_object, parent_move_ctor, { other_object });
    }
    else
    {
      Function parent_copy_ctor = classScope().parent().copyConstructor();
      if (parent_copy_ctor.isNull())
        throw ParentHasNoCopyConstructor{};
      else if (parent_copy_ctor.isDeleted())
        throw ParentHasDeletedCopyConstructor{};

      parent_ctor_call = program::PlacementStatement::New(this_object, parent_copy_ctor, { other_object });
    }
  }

  // Initializating data members
  const auto & data_members = classScope().dataMembers();
  const int data_members_offset = classScope().attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);

    const std::shared_ptr<program::Expression> member_access = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);
    std::shared_ptr<program::Expression> member_value = nullptr;
    if (dm.type.isReference())
      member_value = member_access;
    else
    {
      if (dm.type.isObjectType())
      {
        Function dm_move_ctor = engine()->getClass(dm.type).moveConstructor();
        if (!dm_move_ctor.isNull())
        {
          if (dm_move_ctor.isDeleted())
            throw DataMemberIsNotMovable{};
          member_value = program::ConstructorCall::New(dm_move_ctor, { member_access });
        }
        else
        {
          Function dm_copy_ctor = engine()->getClass(dm.type).copyConstructor();
          if(dm_copy_ctor.isNull() || dm_copy_ctor.isDeleted())
            throw DataMemberIsNotMovable{};
          member_value = program::ConstructorCall::New(dm_copy_ctor, { member_access });
        }
      }
      else
      {
        const ConversionSequence conv = ConversionSequence::compute(member_access, dm.type, engine());
        if (conv == ConversionSequence::NotConvertible())
          throw DataMemberIsNotCopyable{};

        member_value = prepareFunctionArgument(member_access, dm.type, conv);
      }
    }

    members_initialization[i] = program::PushDataMember::New(member_value);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  else
    statements.push_back(program::InitObjectStatement::New(classScope().id()));
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateDestructor()
{
  return generateDestructorFooter();
}


std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateAssignmentOperator()
{
  auto this_object = generateThisAccess();
  auto other_object = program::StackValue::New(1, mStack[1].type);

  std::shared_ptr<program::Statement> parent_assign_call;
  if (!classScope().parent().isNull())
  {
    Operator parent_assign = findAssignmentOperator(classScope().parent().id());
    if (parent_assign.isNull())
      throw ParentHasNoAssignmentOperator{};
    else if (parent_assign.isDeleted())
      throw ParentHasDeletedAssignmentOperator{};

    parent_assign_call = program::ExpressionStatement::New(program::FunctionCall::New(parent_assign, { this_object, other_object }));
  }

  // Assigns data members
  const auto & data_members = classScope().dataMembers();
  const int data_members_offset = classScope().attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_assign{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);
    if (dm.type.isReference())
      throw DataMemberIsReferenceAndCannotBeAssigned{};

    Operator dm_assign = findAssignmentOperator(dm.type);
    if (dm_assign.isNull())
      throw DataMemberHasNoAssignmentOperator{};
    else if (dm_assign.isDeleted())
      throw DataMemberHasDeletedAssignmentOperator{};

    auto fetch_this_member = program::MemberAccess::New(dm.type, this_object, i + data_members_offset);
    auto fetch_other_member = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);

    auto assign = program::FunctionCall::New(dm_assign, { fetch_this_member, fetch_other_member });
    members_assign[i] = program::ExpressionStatement::New(assign);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_assign_call)
    statements.push_back(parent_assign_call);
  statements.insert(statements.end(), members_assign.begin(), members_assign.end());
  statements.insert(statements.end(), program::ReturnStatement::New(this_object));
  return program::CompoundStatement::New(std::move(statements));
}

bool FunctionCompiler::isAssignmentOperator(const Operator & op, const Type & t) const
{
  if (op.operatorId() != Operator::AssignmentOperator)
    return false;

  if (op.returnType() != Type::ref(t.baseType()))
    return false;
  if (op.firstOperand() != Type::ref(t.baseType()))
    return false;
  if (op.secondOperand() != Type::cref(t.baseType()))
    return false;

  return true;
}

Operator FunctionCompiler::findAssignmentOperator(const Type & t)
{
  if (t.isFundamentalType())
  {
    const auto & ops = engine()->rootNamespace().operators();
    for (const auto & o : ops)
    {
      if (isAssignmentOperator(o, t))
        return o;
    }

  }
  else if (t.isEnumType())
  {
    return engine()->getEnum(t).getAssignmentOperator();
  }
  else if (t.isObjectType())
  {
    const auto & ops = engine()->getClass(t).operators();
    for (const auto & o : ops)
    {
      if (isAssignmentOperator(o, t))
        return o;
    }
  }

  return Operator{};
}


Function FunctionCompiler::getDestructor(const Type & t)
{
  if (t.isObjectType())
    return engine()->getClass(t).destructor();
  return Function{};
}

std::shared_ptr<program::LambdaExpression> FunctionCompiler::generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr)
{
  // fetching all captures
  const parser::Token capture_all_by_value = LambdaCompiler::captureAllByValue(*lambda_expr);
  const parser::Token capture_all_by_reference = LambdaCompiler::captureAllByReference(*lambda_expr);
  parser::Token catpure_this = LambdaCompiler::captureThis(*lambda_expr);
  if (!catpure_this.isValid() && canUseThis())
  {
    if (capture_all_by_reference.isValid())
      catpure_this = capture_all_by_reference;
    else 
      catpure_this = capture_all_by_value;
  }

  if (capture_all_by_reference.isValid() && capture_all_by_value.isValid())
    throw CannotCaptureByValueAndByRef{ capture_all_by_reference.line, capture_all_by_reference.column };

  std::vector<bool> capture_flags(mStack.size, false);
  capture_flags[0] = true; // this or return value
  std::vector<Capture> captures;

  const int first_capture_offset = catpure_this.isValid() ? 1 : 0;
  Class captured_class;
  if (catpure_this.isValid())
  {
    if (!canUseThis())
      throw CannotCaptureThis{ catpure_this.line, catpure_this.column };

    std::shared_ptr<program::Expression> this_object = generateThisAccess();

    captures.push_back(Capture{ "this", this_object });
    captured_class = engine()->getClass(this_object->type());
  }

  for (const auto & cap : lambda_expr->captures)
  {
    if (cap.byValueSign.isValid() || (cap.reference.isValid() && !cap.name.isValid()))
      continue;

    const auto & name = lambda_expr->captureName(cap);
    std::shared_ptr<program::Expression> value;
    if (cap.value != nullptr)
      value = generateExpression(cap.value);
    else
    {
      const int offset = mStack.lastIndexOf(name);
      if (offset == -1)
        throw UnknownCaptureName{ cap.name.line, cap.name.column };
      value = program::StackValue::New(offset, mStack.at(offset).type);
      if (!cap.reference.isValid())
      {
        StandardConversion conv = StandardConversion::compute(value->type(), value->type().baseType(), engine());
        if (conv == StandardConversion::NotConvertible())
          throw CannotCaptureNonCopyable{ cap.name.line, cap.name.column };
        value = applyStandardConversion(value, value->type().baseType(), conv);
      }

      capture_flags[offset] = true;
    }

    captures.push_back(Capture{ name, value });
  }

  /// TODO : what about the capture of this ?

  if (capture_all_by_value.isValid())
  {
    for (int i(1 + first_capture_offset); i < mStack.size; ++i)
    {
      if (capture_flags[i])
        continue;
      std::shared_ptr<program::Expression> value = program::StackValue::New(i, mStack.at(i).type);
      StandardConversion conv = StandardConversion::compute(value->type(), value->type().baseType(), engine());
      if (conv == StandardConversion::NotConvertible())
        throw SomeLocalsCannotBeCaptured{ capture_all_by_value.line, capture_all_by_value.column };
      value = applyStandardConversion(value, value->type().baseType(), conv);
      captures.push_back(Capture{ mStack.at(i).name, value });
    }
  }
  else if (capture_all_by_reference.isValid())
  {
    for (int i(1 + first_capture_offset); i < mStack.size; ++i)
    {
      if (capture_flags[i])
        continue;
      auto value = program::StackValue::New(i, mStack.at(i).type);
      captures.push_back(Capture{ mStack.at(i).name, value });
    }
  }

  CompileLambdaTask task;
  task.lexpr = lambda_expr;
  task.capturedObject = captured_class;
  task.scope = currentScope();
  task.captures = std::move(captures);

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
    NameLookup lookup = NameLookup::resolve(std::dynamic_pointer_cast<ast::Identifier>(callee), args, this);

    if (lookup.resultType() == NameLookup::FunctionName)
    {
      std::shared_ptr<program::Expression> object = nullptr;
      if (canUseThis())
        object = program::StackValue::New(1, Type::ref(mStack[1].type));

      OverloadResolution resol = OverloadResolution::New(engine());
      if (!resol.process(lookup.functions(), args, object))
        throw CouldNotFindValidMemberFunction{ call->pos().line, call->pos().col };

      Function selected = resol.selectedOverload();
      if (selected.isDeleted())
        throw CallToDeletedFunction{ call->pos().line, call->pos().col };

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
      throw NotImplementedError{ "FunctionCompiler::generateCall() : call to constructor not handled yet..." };
    }
    
    /// TODO : add error when name does not refer to anything
    throw NotImplementedError{"FunctionCompiler::generateCall() : callee not handled yet"};
  }
  else
    return AbstractExpressionCompiler::generateCall(call);
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & compoundStatement, compiler::FunctionScope::Type scopeType)
{
  auto ret = program::CompoundStatement::New();

  FunctionScope scp{ this, scopeType,  };

  for (const auto & s : compoundStatement->statements)
  {
    ret->statements.push_back(generateStatement(s));
  }

  generateScopeDestruction(scp, ret->statements);

  return ret;
}

std::shared_ptr<program::Statement> FunctionCompiler::generateExpressionStatement(const std::shared_ptr<ast::ExpressionStatement> & es)
{
  auto expr = generateExpression(es->expression);
  return program::ExpressionStatement::New(expr);
}

std::shared_ptr<program::Statement> FunctionCompiler::generateForLoop(const std::shared_ptr<ast::ForLoop> & forLoop)
{
  FunctionScope for_init_scope{this, compiler::FunctionScope::ForInit};

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
  generateScopeDestruction(for_init_scope, statements);

  return program::ForLoop::New(for_init, for_cond, for_loop_incr, body, program::CompoundStatement::New(std::move(statements)));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateIfStatement(const std::shared_ptr<ast::IfStatement> & is)
{
  auto cond = generateExpression(is->condition);
  std::shared_ptr<program::Statement> body;
  if (is->body->is<ast::CompoundStatement>())
    body = generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(is->body), compiler::FunctionScope::IfBody);
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

  const int depth = breakDepth();
  generateScopeDestruction(depth, statements);

  if (js->is<ast::BreakStatement>())
    statements.push_back(program::BreakStatement::New());
  else if (js->is<ast::ContinueStatement>())
    statements.push_back(program::ContinueStatement::New());

  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  const int depth = mScopeStack.size();
  generateScopeDestruction(depth, statements);

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


void FunctionCompiler::generateScopeDestruction(int depth, std::vector<std::shared_ptr<program::Statement>> & statements)
{
  if (depth == 0)
    return;

  const ScopeStack & scpStack = mScopeStack;

  if (scpStack[scpStack.size() - depth].type == compiler::FunctionScope::FunctionBody && isCompilingAnonymousFunction())
    return generateScopeDestruction(depth - 1, statements);

  const Stack & stack = mStack;

  const int scope_offset = scpStack[scpStack.size() - depth].offset;
  for (int i(stack.size - 1); i >= scope_offset; --i)
    statements.push_back(generateVariableDestruction(stack.at(i)));

  // destroying default arguments
  if (scpStack[scpStack.size() - depth].type == compiler::FunctionScope::FunctionBody)
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

void FunctionCompiler::generateScopeDestruction(compiler::FunctionScope & scp, std::vector<std::shared_ptr<program::Statement>> & statements)
{
  Stack & stack = mStack;

  for (int i(stack.size - 1); i >= scp.sp(); --i)
    statements.push_back(generateVariableDestruction(stack.at(i)));
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
    throw TemplateNamesAreNotExpressions{ identifier->pos().line, identifier->pos().col };
  case NameLookup::TypeName:
    throw TypeNameInExpression{ identifier->pos().line, identifier->pos().col };
  case NameLookup::VariableName:
    return program::Literal::New(lookup.variable()); /// TODO : perhaps a VariableAccess would be better
  case NameLookup::DataMemberName:
    return generateMemberAccess(lookup.dataMemberIndex());
  case NameLookup::GlobalName:
    return generateGlobalAccess(lookup.globalIndex());
  case NameLookup::LocalName:
    return generateLocalVariableAccess(lookup.localIndex());
  case NameLookup::EnumValueName:
    return program::Literal::New(Value::fromEnumValue(lookup.enumValueResult()));
  case NameLookup::NamespaceName:
    throw NamespaceNameInExpression{ identifier->pos().line, identifier->pos().col };
  default:
    break;
  }

  throw NotImplementedError{ "FunctionCompiler::generateVariableAccess() : kind of name not supported yet" };
}

std::shared_ptr<program::Expression> FunctionCompiler::generateThisAccess()
{
  if(mFunction.isDestructor() || mFunction.isConstructor())
    return program::StackValue::New(0, Type::ref(mStack[0].type));
  return program::StackValue::New(1, Type::ref(mStack[1].type));
}

std::shared_ptr<program::Expression> FunctionCompiler::generateMemberAccess(const int index)
{
  /// TODO : this and its overload need some refactoring...
  /// they also need correct const-ref qualification for ex. when the object is const

  Class cla = classScope();
  int relative_index = index;
  while (relative_index - int(cla.dataMembers().size()) >= 0)
  {
    relative_index = relative_index - cla.dataMembers().size();
    cla = cla.parent();
  }

  auto this_object = generateThisAccess();
  const Type member_type = cla.dataMembers().at(index).type;
  return program::MemberAccess::New(member_type, this_object, index);
}

std::shared_ptr<program::Expression> FunctionCompiler::generateMemberAccess(const std::shared_ptr<program::Expression> & object, const int index)
{
  Class cla = engine()->getClass(object->type());
  int relative_index = index;
  while (relative_index - cla.dataMembers().size() >= 0)
  {
    relative_index = relative_index - cla.dataMembers().size();
    cla = cla.parent();
  }

  const Type member_type = cla.dataMembers().at(index).type;
  return program::MemberAccess::New(member_type, object, index);
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
  if (var_type.isReference() || var_type.isRefRef())
    throw ReferencesMustBeInitialized{};

  if (var_type.isFundamentalType())
    return generateFundamentalVariableCreation(var_type, var_decl->name->getName());
  else if (var_type.isEnumType())
    throw EnumerationsMustBeInitialized{ var_decl->pos().line, var_decl->pos().col };
  else if (var_type.isFunctionType())
    throw FunctionVariablesMustBeInitialized{ var_decl->pos().line, var_decl->pos().col };
  else if (var_type.isObjectType())
  {
    Class cla = engine()->getClass(var_type);
    Function ctor = cla.defaultConstructor();
    if (ctor.isNull())
      throw VariableCannotBeDefaultConstructed{};
    else if (ctor.isDeleted())
      throw VariableCannotBeDestroyed{}; /// TODO : move in generateScopeDestruction()

    return generateVariableCreation(var_type, var_decl->name->getName(), program::ConstructorCall::New(ctor, {}));
  }
  else
    throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::ConstructorInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ var_decl->pos().line, var_decl->pos().col };

  if (!var_type.isObjectType() && init->args.size() != 1)
    throw TooManyArgumentInVariableInitialization{ var_decl->pos().line, var_decl->pos().col };

  if((var_type.isReference() || var_type.isRefRef()) && init->args.size() != 1)
    throw TooManyArgumentInReferenceInitialization{ var_decl->pos().line, var_decl->pos().col };

  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(init->args);
  
  if (var_type.isReference() && !var_type.isConst())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), var_type, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ init->args.front()->pos().line, init->args.front()->pos().col,
      engine()->typeName(args.front()->type()), engine()->typeName(var_type) };

    return generateVariableCreation(var_type, var_decl->name->getName(), prepareFunctionArgument(args.front(), var_type, seq));
  }

  if (var_type.isFundamentalType())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), var_type, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ init->args.front()->pos().line, init->args.front()->pos().col,
        engine()->typeName(args.front()->type()), engine()->typeName(var_type) };

    /// TODO : this is not optimal I believe
    auto value = prepareFunctionArgument(args.front(), var_type, seq);
    return generateVariableCreation(var_type, var_decl->name->getName(), value);
  }
  else if (var_type.isEnumType())
  {
    if(args.front()->type().baseType() != var_type.baseType())
      throw CouldNotConvert{ init->args.front()->pos().line, init->args.front()->pos().col,
      engine()->typeName(args.front()->type()), engine()->typeName(var_type) };

    return generateVariableCreation(var_type, var_decl->name->getName(), program::Copy::New(var_type, args.front()));
  }
  else if (var_type.isObjectType())
  {
    const std::vector<Function> & ctors = engine()->getClass(var_type).constructors();
    OverloadResolution resol = OverloadResolution::New(engine());
    if (!resol.process(ctors, args))
      throw CouldNotFindValidConstructor{ var_decl->pos().line, var_decl->pos().col }; /// TODO add a better diagnostic message

    const Function ctor = resol.selectedOverload();
    const auto & conversions = resol.conversionSequence();

    prepareFunctionArguments(args, ctor.prototype(), conversions);

    /// TODO : handle initialization of references

    auto ctor_call = program::ConstructorCall::New(ctor, std::move(args));
    return generateVariableCreation(var_type, var_decl->name->getName(), ctor_call);
  }
  else
    throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::BraceInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw AutoMustBeUsedWithAssignment{ var_decl->pos().line, var_decl->pos().col };

  if (!var_type.isObjectType() && init->args.size() != 1)
    throw TooManyArgumentInVariableInitialization{ var_decl->pos().line, var_decl->pos().col };

  if ((var_type.isReference() || var_type.isRefRef()) && init->args.size() != 1)
    throw TooManyArgumentInReferenceInitialization{ var_decl->pos().line, var_decl->pos().col };

  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(init->args);

  if (var_type.isReference() && !var_type.isConst())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), var_type, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ init->args.front()->pos().line, init->args.front()->pos().col,
      engine()->typeName(args.front()->type()), engine()->typeName(var_type) };

    return generateVariableCreation(var_type, var_decl->name->getName(), prepareFunctionArgument(args.front(), var_type, seq));
  }

  if (var_type.isFundamentalType())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), var_type, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ init->args.front()->pos().line, init->args.front()->pos().col,
      engine()->typeName(args.front()->type()), engine()->typeName(var_type) };

    if (seq.isNarrowing())
      throw NarrowingConversionInBraceInitialization{ var_decl->pos().line, var_decl->pos().col };

    /// TODO : this is not optimal I believe
    auto value = prepareFunctionArgument(args.front(), var_type, seq);
    return generateVariableCreation(var_type, var_decl->name->getName(), value);
  }
  else if (var_type.isEnumType())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), var_type, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ init->args.front()->pos().line, init->args.front()->pos().col,
      engine()->typeName(args.front()->type()), engine()->typeName(var_type) };

    return generateVariableCreation(var_type, var_decl->name->getName(), prepareFunctionArgument(args.front(), var_type, seq));
  }
  else if (var_type.isObjectType())
  {
    const std::vector<Function> & ctors = engine()->getClass(var_type).constructors();
    OverloadResolution resol = OverloadResolution::New(engine());
    if (!resol.process(ctors, args))
      throw CouldNotFindValidConstructor{ var_decl->pos().line, var_decl->pos().col }; /// TODO add a better diagnostic message

    const Function ctor = resol.selectedOverload();
    const auto & conversions = resol.conversionSequence();
    for (const auto & conv : conversions)
    {
      if (conv.isNarrowing())
        throw NarrowingConversionInBraceInitialization{ var_decl->pos().line, var_decl->pos().col };
    }

    prepareFunctionArguments(args, ctor.prototype(), conversions);

    /// TODO : handle initialization of references

    auto ctor_call = program::ConstructorCall::New(ctor, std::move(args));
    return generateVariableCreation(var_type, var_decl->name->getName(), ctor_call);
  }
  else
    throw std::runtime_error{ "Not implemented" };
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
    throw CouldNotConvert{ init->pos().line, init->pos().col,
    engine()->typeName(value->type()), engine()->typeName(var_type) };
  
  /// TODO : this is not optimal I believe
  // we could add copy elision
  value = prepareFunctionArgument(value, var_type, seq);
  return generateVariableCreation(var_type, var_decl->name->getName(), value);
}

std::shared_ptr<program::Expression> FunctionCompiler::constructFundamentalValue(const Type & t, bool copy)
{
  Value val;
  switch (t.baseType().data())
  {
  case Type::Null:
  case Type::Void:
    throw std::runtime_error{ "Invalid variable type" };
  case Type::Boolean:
    val = engine()->newBool(false);
    break;
  case Type::Char:
    val = engine()->newChar('\0');
    break;
  case Type::Int:
    val = engine()->newInt(0);
    break;
  case Type::Float:
    val = engine()->newFloat(0.f);
    break;
  case Type::Double:
    val = engine()->newDouble(0.);
    break;
  default:
    throw std::runtime_error{ "Not implemented" };
  }

  engine()->manage(val);

  auto lit = program::Literal::New(val);
  if (copy)
    return program::Copy::New(t, lit);
  return lit;
}


std::shared_ptr<program::Statement> FunctionCompiler::generateFundamentalVariableCreation(const Type & type, const std::string & name)
{
  return generateVariableCreation(type, name, constructFundamentalValue(type, true));
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableCreation(const Type & type, const std::string & name, const std::shared_ptr<program::Expression> & value)
{
  const int stack_index = mStack.addVar(type, name);

  auto var_decl = program::PushValue::New(type, name, value, stack_index);

  if (mScopeStack.back().type == compiler::FunctionScope::FunctionBody && isCompilingAnonymousFunction())
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


std::shared_ptr<program::Statement> FunctionCompiler::generateVariableInitialization(const Variable & var)
{
  throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableInitialization(const Variable & var, const std::vector<std::shared_ptr<program::Expression>> & args)
{
  throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Statement> FunctionCompiler::generateVariableInitialization(const std::shared_ptr<program::Expression> & var, int type, const std::vector<std::shared_ptr<program::Expression>> & args)
{
  throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Statement> FunctionCompiler::generateWhileLoop(const std::shared_ptr<ast::WhileLoop> & whileLoop)
{
  auto cond = generateExpression(whileLoop->condition);

  /// TODO : convert to bool if not bool
  if (cond->type().baseType() != Type::Boolean)
    throw NotImplementedError{ "FunctionCompiler::generateWhileLoop() : implicit conversion to bool not implemented" };

  std::shared_ptr<program::Statement> body;
  if (whileLoop->body->is<ast::CompoundStatement>())
    body = generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(body), compiler::FunctionScope::WhileBody);
  else
    body = generateStatement(whileLoop->body);

  return program::WhileLoop::New(cond, body);
}

} // namespace compiler

} // namespace script

