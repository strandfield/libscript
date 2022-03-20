// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/functioncompiler.h"
#include "script/private/functionscope_p.h"

#include "script/compiler/compiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/debug-info.h"
#include "script/compiler/diagnostichelper.h"

#include "script/compiler/assignmentcompiler.h"
#include "script/compiler/constructorcompiler.h"
#include "script/compiler/destructorcompiler.h"
#include "script/compiler/lambdacompiler.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/valueconstructor.h"

#include "script/ast/ast_p.h"
#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/functiontype.h"
#include "script/namelookup.h"
#include "script/templateargumentprocessor.h"
#include "script/typesystem.h"

#include "script/private/function_p.h"
#include "script/private/namelookup_p.h"
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
  mSp = static_cast<int>(mCompiler->mStack.size());
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
  /// TODO: (update), remove this block, it is useless
  /// TODO : this block is incorrect, 'this' is not always at index 1 !
  if (name == "this")
  {
    if (!mCompiler->canUseThis())
      throw CompilationFailure{ CompilerError::IllegalUseOfThis };

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

int FunctionScope::add_var(utils::StringView name, const Type & t)
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
  : index(std::numeric_limits<size_t>::max())
  , global(false)
  , is_static(false)
{

}

Variable::Variable(const Type & t, utils::StringView n, size_t i, bool g, bool s)
  : type(t)
  , name(std::move(n))
  , index(i)
  , global(g)
  , is_static(false)
{

}

int Stack::addVar(const Type & t, utils::StringView name)
{
  this->data.push_back(Variable(t, name, this->data.size()));

  if (enable_debug)
  {
    debuginfo = std::make_shared<DebugInfoBlock>(t, name.toString(), debuginfo);
  }

  return static_cast<int>(this->data.size()) - 1;
}

int Stack::indexOf(const std::string & var) const
{
  for (size_t i(0); i < data.size(); ++i)
  {
    if (this->data[i].name == var)
      return static_cast<int>(i);
  }

  return -1;
}

int Stack::lastIndexOf(const std::string & var) const
{
  return lastIndexOf(utils::StringView(var.data(), var.size()));
}

int Stack::lastIndexOf(utils::StringView var) const
{
  for (size_t i(data.size()); i-- > 0; )
  {
    if (this->data[i].name == var)
      return static_cast<int>(i);
  }

  return -1;
}

void Stack::destroy(size_t n)
{
  assert(n <= data.size());

  while (n-- > 0)
  {
    data.pop_back();

    if (enable_debug && debuginfo)
      debuginfo = debuginfo->prev;
  }
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


FunctionCompilerExtension::FunctionCompilerExtension(FunctionCompiler* c)
  : Component(c->compiler()),
   m_function_compiler(c)
{

}

Class FunctionCompilerExtension::currentClass() const 
{ 
  return m_function_compiler->classScope(); 
}

FunctionCompiler* FunctionCompilerExtension::compiler() const 
{
  return m_function_compiler;
}

const std::shared_ptr<ast::Declaration> & FunctionCompilerExtension::declaration() const
{ 
  return m_function_compiler->declaration();
}

Engine * FunctionCompilerExtension::engine() const 
{ 
  return m_function_compiler->engine();
}

Stack & FunctionCompilerExtension::stack()
{
  return m_function_compiler->mStack;
}

ExpressionCompiler & FunctionCompilerExtension::ec() 
{ 
  return m_function_compiler->expr_;
}

std::string FunctionCompilerExtension::dstr(const std::shared_ptr<ast::Identifier> & id)
{ 
  return diagnostic::dstr(id); 
}

NameLookup FunctionCompilerExtension::resolve(const std::shared_ptr<ast::Identifier> & name)
{
  return m_function_compiler->resolve(name);
}



FunctionCompiler::FunctionCompiler(Compiler *c)
  : Component(c)
  , expr_{c}
  , modules_(c)
{
  if (c->hasActiveSession())
    setCompileMode(c->session()->compile_mode);

  expr_.variableAccessor().setStack(&mStack);

  expr_.setStack(&mStack);

  scope_statements_.scope_ = &mCurrentScope;
}

FunctionCompiler::~FunctionCompiler()
{

}

script::CompileMode FunctionCompiler::compileMode() const
{
  return mCompileMode;
}

void FunctionCompiler::setCompileMode(script::CompileMode cm)
{
  if (cm != mCompileMode)
  {
    mCompileMode = cm;
    mStack.enable_debug = (cm == script::CompileMode::Debug);
  }
}

bool FunctionCompiler::isDebugCompilation() const
{
  return compileMode() == script::CompileMode::Debug;
}

void FunctionCompiler::compile(const CompileFunctionTask & task)
{
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
  mFunction.impl()->set_body(body);
}


Script FunctionCompiler::script()
{
  return mFunction.script();
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

utils::StringView FunctionCompiler::argumentName(int index) const
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
  return mFunction.hasImplicitObject();
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
      body->statements.push_back(program::ReturnStatement::New(expr_.implicit_object()));
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

  TranslationTarget target{ this, mDeclaration };

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

  throw CompilationFailure{ CompilerError::FunctionCannotBeDefaulted };
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
  return ConstructorCompiler::generateDefaultConstructor(classScope());
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateCopyConstructor()
{
  return ConstructorCompiler::generateCopyConstructor(classScope());
}

std::shared_ptr<program::CompoundStatement> FunctionCompiler::generateMoveConstructor()
{
  return ConstructorCompiler::generateMoveConstructor(classScope());
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
  const auto size = buffer_size();

  processCompoundStatement(cs, scopeType);

  return program::CompoundStatement::New(resize_buffer(size));
}

void FunctionCompiler::process(const std::shared_ptr<ast::Statement> & s)
{
  TranslationTarget target{ this, s };

  insertBreakpoint(*s);

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
  case ast::NodeType::ClassDeclaration:
  case ast::NodeType::EnumDeclaration:
    // @TODO: throw exception, classes and enums can not be defined inside functions
    break;
  default:
    break;
  }

  assert(false);
  throw std::runtime_error{ "FunctionCompiler::process() : not implemented" };
}

void FunctionCompiler::processExitScope(const Scope & scp, const ast::Statement& s)
{
  assert(dynamic_cast<const FunctionScope *>(scp.impl().get()) != nullptr);

  const FunctionScope *fscp = dynamic_cast<const FunctionScope *>(scp.impl().get());

  const int sp = fscp->sp();
  Stack & stack = mStack;

  for (size_t i(stack.size()); i-- > sp; )
  {
    insertExitBreakpoint(stack.size() - 1 - i, s);
    processVariableDestruction(stack.at(i));
  }
}

void FunctionCompiler::generateExitScope(const Scope & scp, std::vector<std::shared_ptr<program::Statement>> & statements, const ast::Statement& s)
{
  BufferSwap swap{ mBuffer, statements };
  processExitScope(scp, s);
}

void FunctionCompiler::insertBreakpoint(const ast::Statement& s)
{
  if (!isDebugCompilation())
    return;

  // @TODO: optimize! map() is O(n)
  utils::StringView tok = s.base_token().text();
  size_t off = std::distance(mFunction.script().source().content().data(), tok.data());
  SourceFile::Position pos = mFunction.script().source().map(off);
  int line = pos.line;

  auto bp = std::make_shared<program::Breakpoint>(line, mStack.debuginfo);
  mFunction.script().impl()->add_breakpoint(mFunction, bp);
  write(bp);
}

void FunctionCompiler::insertExitBreakpoint(size_t delta, const ast::Statement& s)
{
  if (!isDebugCompilation())
    return;

  // @TODO: optimize! map() is O(n)
  utils::StringView src = s.source();
  size_t off = std::distance(mFunction.script().source().content().data(), src.data()) + src.size() - 1;
  SourceFile::Position pos = mFunction.script().source().map(off);
  int line = pos.line;

  auto bp = std::make_shared<program::Breakpoint>(line, DebugInfoBlock::fetch(mStack.debuginfo, delta));
  mFunction.script().impl()->add_breakpoint(mFunction, bp);
  write(bp);
}

void FunctionCompiler::processCompoundStatement(const std::shared_ptr<ast::CompoundStatement> & cs, FunctionScope::Category scopeType)
{
  TranslationTarget target{ this, cs };
  EnterScope guard{ this, scopeType };

  for (const auto & s : cs->statements)
    process(s);

  processExitScope(mCurrentScope, *cs);
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
    throw NotImplemented{ "Implicit conversion to bool not implemented yet in for-condition" };
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
  generateExitScope(mCurrentScope, statements, *fl);

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
  if (id->export_keyword.isValid() && !isCompilingAnonymousFunction())
  {
    // TODO: create error code for this
    log(DiagnosticMessage{ diagnostic::Severity::Error, EngineError::NotImplemented, "'export' are only allowed at script level" });
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
    generateExitScope(scp, statements, *js);
    write(program::BreakStatement::New(std::move(statements)));
    return;
  }
  else if (js->is<ast::ContinueStatement>())
  {
    const Scope scp = continueScope();
    generateExitScope(scp, statements, *js);
    write(program::ContinueStatement::New(std::move(statements)));
    return;
  }

  assert(false);
  throw NotImplemented{ "This kind of jump statement not implemented" };
}

void FunctionCompiler::processReturnStatement(const std::shared_ptr<ast::ReturnStatement> & rs)
{
  std::vector<std::shared_ptr<program::Statement>> statements;

  generateExitScope(mFunctionBodyScope, statements, *rs);

  if (rs->expression == nullptr)
  {
    if (mFunction.prototype().returnType() != Type::Void)
      throw CompilationFailure{ CompilerError::ReturnStatementWithoutValue };

    return write(program::ReturnStatement::New(nullptr, std::move(statements)));
  }
  else
  {
    if (mFunction.prototype().returnType() == Type::Void)
      throw CompilationFailure{ CompilerError::ReturnStatementWithValue };
  }

  auto retval = generate(rs->expression);

  const Conversion conv = Conversion::compute(retval, mFunction.prototype().returnType(), engine());

  if (conv == Conversion::NotConvertible())
  {
    throw CompilationFailure{ CompilerError::CouldNotConvert, errors::ConversionFailure{retval->type(), mFunction.prototype().returnType()} };
  }

  /// TODO : write a dedicated function for this, don't use prepareFunctionArg()
  retval = ConversionProcessor::convert(engine(), retval, conv);

  write(program::ReturnStatement::New(retval, std::move(statements)));
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl)
{
  const Type var_type = script::compiler::resolve_type(var_decl->variable_type, mCurrentScope);

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
    throw CompilationFailure{ CompilerError::AutoMustBeUsedWithAssignment };

  try
  {
    expr_.setScope(mCurrentScope);
    processVariableCreation(var_decl, var_type, ValueConstructor::construct(engine(), var_type, nullptr));
  }
  catch (const CompilationFailure & ex)
  {
    if (ex.errorCode() == CompilerError::EnumerationsCannotBeDefaultConstructed)
      throw CompilationFailure{ CompilerError::EnumerationsMustBeInitialized };
    else
      throw;
  }
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::ConstructorInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw CompilationFailure{ CompilerError::AutoMustBeUsedWithAssignment };

  try
  {
    expr_.setScope(mCurrentScope);
    processVariableCreation(var_decl, var_type, ValueConstructor::construct(expr_, var_type, init));
  }
  catch (const CompilationFailure& ex)
  {
    if (ex.errorCode() == CompilerError::TooManyArgumentInInitialization)
      throw CompilationFailure{ CompilerError::TooManyArgumentInVariableInitialization };
    else
      throw;
  }
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & var_type, const std::shared_ptr<ast::BraceInitialization> & init)
{
  if (var_type.baseType() == Type::Auto)
    throw CompilationFailure{ CompilerError::AutoMustBeUsedWithAssignment };

  try
  {
    expr_.setScope(mCurrentScope);
    processVariableCreation(var_decl, var_type, ValueConstructor::construct(expr_, var_type, init));
  }
  catch (const CompilationFailure& ex)
  {
    if (ex.errorCode() == CompilerError::TooManyArgumentInInitialization)
      throw CompilationFailure{ CompilerError::TooManyArgumentInVariableInitialization };
    else
      throw;
  }
}

void FunctionCompiler::processVariableDeclaration(const std::shared_ptr<ast::VariableDecl> & var_decl, const Type & input_var_type, const std::shared_ptr<ast::AssignmentInitialization> & init)
{
  auto value = generate(init->value);

  Type var_type = input_var_type;
  if (input_var_type.baseType() == Type::Auto)
  {
    if (value->is<program::InitializerList>())
      return processVariableInitListDecl(var_decl, std::static_pointer_cast<program::InitializerList>(value));

    var_type = value->type();
    if (var_type.isConst())
      var_type = var_type.withFlag(Type::ConstFlag);
    else if (var_type.isReference())
      var_type = var_type.withFlag(Type::ReferenceFlag);
  }

  /// TODO: use Initialization instead
  Conversion conv = Conversion::compute(value, var_type, engine());

  if (conv == Conversion::NotConvertible())
    throw CompilationFailure{ CompilerError::CouldNotConvert, errors::ConversionFailure{value->type(), var_type} };

  /// TODO : this is not optimal I believe
  // we could add copy elision
  value = ConversionProcessor::convert(engine(), value, conv);
  processVariableCreation(var_decl, var_type, value);
}

void FunctionCompiler::processVariableInitListDecl(const std::shared_ptr<ast::VariableDecl> & varDecl, const std::shared_ptr<program::InitializerList> & initlist)
{
  throw NotImplemented{ "Initializer list variables not implemented yet" };
}

void FunctionCompiler::processVariableCreation(const std::shared_ptr<ast::VariableDecl>& var_decl, const Type & type, const std::shared_ptr<program::Expression> & value)
{
  const int stack_index = std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->add_var(var_decl->name->name.text(), type);

  if (!var_decl->staticSpecifier.isValid())
  {
    write(program::PushValue::New(type, var_decl->name->getName(), value, stack_index));
  }
  else
  {
    mStack[stack_index].is_static = true;

    auto simpl = script().impl();

    write(program::PushStaticValue::New(var_decl->name->getName(), script().id(), simpl->static_variables.size(), value));

    simpl->static_variables.push_back(Value());
  }

  if (std::dynamic_pointer_cast<FunctionScope>(mCurrentScope.impl())->category() == FunctionScope::FunctionBody && isCompilingAnonymousFunction())
  {
    mStack[stack_index].global = true;

    auto simpl = script().impl();
    simpl->register_global(Type::ref(mStack[stack_index].type), mStack[stack_index].name.toString());

    write(program::PushGlobal::New(script().id(), stack_index));
  }
}

void FunctionCompiler::processVariableDestruction(const Variable & var)
{
  if (var.global || var.is_static)
    return write(program::PopValue::New(false, Function{}, static_cast<int>(var.index)));

  const bool is_ref = var.type.isReference() || var.type.isRefRef();
  const bool destroy = !is_ref;

  if (var.type.isObjectType())
  {
    Function dtor = engine()->typeSystem()->getClass(var.type).destructor();
    if (dtor.isNull())
      throw CompilationFailure{ CompilerError::ObjectHasNoDestructor };

    return write(program::PopValue::New(destroy, dtor, static_cast<int>(var.index)));
  }

  write(program::PopValue::New(destroy, Function{}, static_cast<int>(var.index)));
}

void FunctionCompiler::processWhileLoop(const std::shared_ptr<ast::WhileLoop> & whileLoop)
{
  auto cond = generate(whileLoop->condition);

  /// TODO : convert to bool if not bool
  if (cond->type().baseType() != Type::Boolean)
    throw NotImplemented{ "FunctionCompiler::generateWhileLoop() : implicit conversion to bool not implemented" };

  std::shared_ptr<program::Statement> body;
  if (whileLoop->body->is<ast::CompoundStatement>())
    body = generateCompoundStatement(std::dynamic_pointer_cast<ast::CompoundStatement>(whileLoop->body), FunctionScope::WhileBody);
  else
    body = generate(whileLoop->body);

  write(program::WhileLoop::New(cond, body));
}

} // namespace compiler

} // namespace script

