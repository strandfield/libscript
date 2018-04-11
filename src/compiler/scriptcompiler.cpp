// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/scriptcompiler.h"

#include "script/compiler/compilererrors.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functioncompiler.h"

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/program/expression.h"

#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "../class_p.h"
#include "../engine_p.h"
#include "../enum_p.h"
#include "../function_p.h"
#include "script/functiontype.h"
#include "script/literals.h"
#include "../literals_p.h"
#include "../scope_p.h"
#include "../script_p.h"

namespace script
{

namespace compiler
{



StateLock::StateLock(ScriptCompiler *c)
  : compiler(c)
{
  this->scope = compiler->currentScope();
  this->decl = compiler->currentDeclaration();
}

StateLock::~StateLock()
{
  compiler->setCurrentDeclaration(this->decl, ScriptCompiler::StatePasskey{});
  compiler->setCurrentScope(this->scope, ScriptCompiler::StatePasskey{});
}


ScriptCompiler::ScriptCompiler(Compiler *c, CompileSession *s)
  : CompilerComponent(c, s)
{

}

void ScriptCompiler::compile(const CompileScriptTask & task)
{
  mScript = task.script;
  mAst = task.ast;
  mCurrentScope = script::Scope{ mScript, Scope{engine()->rootNamespace()} };

  assert(mAst != nullptr);

  Function program = registerRootFunction();
  processFirstOrderDeclarationsAndCollectHigherOrderDeclarations();
  processSecondOrderDeclarations();
  processThirdOrderDeclarations();
  compileFunctions();
  initializeStaticVariables();

  mScript.implementation()->program = program;
}

NameLookup ScriptCompiler::resolve(const std::shared_ptr<ast::Identifier> & identifier)
{
  return NameLookup::resolve(identifier, currentScope());
}

std::string ScriptCompiler::repr(const std::shared_ptr<ast::Identifier> & id)
{
  if (id->type() == ast::NodeType::SimpleIdentifier)
    return id->getName();

  /// TODO : implemented other identifier types

  return id->getName();
}

/// TODO : remove this ugly duplicate of AbstractExpressionCompiler
Type ScriptCompiler::resolveFunctionType(const ast::QualifiedType & qt)
{
  Prototype proto;
  proto.setReturnType(resolve(qt.functionType->returnType));

  for (const auto & p : qt.functionType->params)
    proto.addArgument(resolve(p));

  auto ft = engine()->getFunctionType(proto);
  Type t = ft.type();
  if (qt.constQualifier.isValid())
    t = t.withFlag(Type::ConstFlag);
  if (qt.reference == parser::Token::Ref)
    t = t.withFlag(Type::ReferenceFlag);
  else if (qt.reference == parser::Token::RefRef)
    t = t.withFlag(Type::ForwardReferenceFlag);
  return t;
}

Type ScriptCompiler::resolve(const ast::QualifiedType & qt)
{
  if (qt.functionType != nullptr)
    return resolveFunctionType(qt);

  NameLookup lookup = NameLookup::resolve(qt.type, currentScope());
  if (lookup.resultType() != NameLookup::TypeName)
    throw InvalidTypeName{ dpos(qt.type), repr(qt.type) };

  Type t = lookup.typeResult();
  if (qt.constQualifier.isValid())
    t.setFlag(Type::ConstFlag);
  if (qt.isRef())
    t.setFlag(Type::ReferenceFlag);
  else if (qt.isRefRef())
    t.setFlag(Type::ForwardReferenceFlag);

  return t;
}

Function ScriptCompiler::registerRootFunction()
{
  auto scriptfunc = std::make_shared<ScriptFunctionImpl>(engine());
  scriptfunc->script = mScript.weakref();
  
  auto fakedecl = ast::FunctionDecl::New(std::shared_ptr<ast::AST>());
  fakedecl->body = ast::CompoundStatement::New(parser::Token{ parser::Token::LeftBrace, 0, 0, 0, 0 }, parser::Token{ parser::Token::RightBrace, 0, 0, 0, 0 });
  const auto & stmts = mAst->statements();
  for (const auto & s : stmts)
  {
    switch (s->type())
    {
    case ast::NodeType::ClassDeclaration:
    case ast::NodeType::EnumDeclaration:
    case ast::NodeType::Typedef:
    case ast::NodeType::FunctionDeclaration:
    case ast::NodeType::OperatorOverloadDeclaration:
      break;
    case ast::NodeType::CastDeclaration:
      throw NotImplementedError{ "ScriptCompiler::registerRootFunction() : cast declaration are not allowed at this scope" };
    default:
      fakedecl->body->statements.push_back(s);
      break;
    }
  }

  schedule(CompileFunctionTask{ Function{scriptfunc}, fakedecl, currentScope() });

  return scriptfunc;
}

bool ScriptCompiler::processFirstOrderDeclarationsAndCollectHigherOrderDeclarations()
{
  for (const auto & decl : mAst->declarations())
    processOrCollectDeclaration(decl, mCurrentScope);

  return true;
}

void ScriptCompiler::processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const script::Scope & scope)
{
  StateLock lock{ this }; // backs up current declaration and scope
  this->setState(scope, declaration);

  if (isFirstOrderDeclaration(declaration))
  {
    switch (declaration->type())
    {
    case ast::NodeType::ClassDeclaration:
      processClassDeclaration();
      break;
    case ast::NodeType::EnumDeclaration:
      processEnumDeclaration();
      break;
    case ast::NodeType::Typedef:
      processTypedef();
      break;
      //case ast::NodeType::TemplateDeclaration:
      //  processFirstOrderTemplateDeclaration();
      //  break;
    default:
      throw NotImplementedError{"This kind of declaration is not implemented yet"};
    }
  }
  else if (isSecondOrderDeclaration(declaration))
    mSecondOrderDeclarations.push_back(Declaration{ scope, declaration });
  else if (isThirdOrderDeclaration(declaration))
    mThirdOrderDeclarations.push_back(Declaration{ scope, declaration });
  else
    throw NotImplementedError{ "This kind of declaration is not implemented yet" };

  // the lock will restore the previous state 
  // thanks RAII !
}


void ScriptCompiler::processSecondOrderDeclarations()
{
  bool processed_one = true;

  while (!mSecondOrderDeclarations.empty() && processed_one)
  {
    processed_one = false;

    for (size_t i(0); i < mSecondOrderDeclarations.size(); ++i)
    {
      if (processSecondOrderDeclaration(mSecondOrderDeclarations.at(i)))
      {
        processed_one = true;
        mSecondOrderDeclarations.erase(mSecondOrderDeclarations.begin() + i);
        --i;
      }
    }
  }

  if (!mSecondOrderDeclarations.empty())
    throw DeclarationProcessingError{};
}


bool ScriptCompiler::processSecondOrderDeclaration(const compiler::Declaration & decl)
{
  StateLock lock{ this };
  this->setState(decl.scope, decl.declaration);

  switch (decl.declaration->type())
  {
  case ast::NodeType::FunctionDeclaration:
    return processFunctionDeclaration();
  case ast::NodeType::ConstructorDeclaration:
    return processConstructorDeclaration();
  case ast::NodeType::DestructorDeclaration:
    return processDestructorDeclaration();
  case ast::NodeType::OperatorOverloadDeclaration:
    return processOperatorOverloadingDeclaration();
  case ast::NodeType::CastDeclaration:
    return processCastOperatorDeclaration();
    //case ast::NodeType::TemplateDeclaration:
    //  return processSecondOrderTemplateDeclaration();
  default:
    break;
  }

  throw NotImplementedError{"ScriptCompiler::processSecondOrderDeclaration() : implementation error"};
}

void ScriptCompiler::processDataMemberDecl(const std::shared_ptr<ast::VariableDecl> & var_decl)
{
  Class current_class = currentScope().asClass();
  assert(!current_class.isNull());

  if (var_decl->variable_type.type->name == parser::Token::Auto)
    throw DataMemberCannotBeAuto{ dpos(var_decl) };

  /// TODO : consider variable which are functions
  if (var_decl->variable_type.functionType != nullptr)
    throw NotImplementedError{ "Static variables of function-type are not implemented yet" };

  /// TODO : consider const-ref qualifications 
  NameLookup lookup = NameLookup::resolve(var_decl->variable_type.type, currentScope());
  if (lookup.resultType() != NameLookup::TypeName)
    throw InvalidTypeName{ dpos(var_decl), repr(var_decl->variable_type.type) };

  if (var_decl->staticSpecifier.isValid())
  {
    if (var_decl->variable_type.type->name == parser::Token::Auto)
      throw DataMemberCannotBeAuto{ dpos(var_decl) };

    if (var_decl->init == nullptr)
      throw MissingStaticInitialization{ dpos(var_decl) };

    if (var_decl->init->is<ast::ConstructorInitialization>() || var_decl->init->is<ast::BraceInitialization>())
      throw InvalidStaticInitialization{ dpos(var_decl) };

    auto expr = var_decl->init->as<ast::AssignmentInitialization>().value;
    if (lookup.typeResult().isFundamentalType() && expr->is<ast::Literal>() && !expr->is<ast::UserDefinedLiteral>())
    {
      auto exprcomp = std::unique_ptr<ExpressionCompiler>(getComponent<ExpressionCompiler>());
      auto execexpr = exprcomp->compile(expr, currentScope());
      
      Value val = engine()->implementation()->interpreter->eval(execexpr);
      current_class.addStaticDataMember(var_decl->name->getName(), val);
    }
    else
    {
      Value staticMember = current_class.implementation()->add_static_data_member(var_decl->name->getName(), lookup.typeResult());
      mStaticVariables.push_back(StaticVariable{ staticMember, expr, currentScope() });
    }
  }
  else
  {
    Class::DataMember dataMember;
    dataMember.name = var_decl->name->getName();
    dataMember.type = lookup.typeResult();

    current_class.implementation()->dataMembers.push_back(dataMember);
  }
}

void ScriptCompiler::processThirdOrderDeclarations()
{
  for (size_t i(0); i < mThirdOrderDeclarations.size(); ++i)
  {
    const auto & decl = mThirdOrderDeclarations.at(i);
    StateLock lock{ this };
    this->setState(decl.scope, decl.declaration);

    if (currentScope().type() == Scope::ScriptScope)
    {
      // Global variables are processed by the function compiler
      continue;
    }

    std::shared_ptr<ast::VariableDecl> var_decl = std::dynamic_pointer_cast<ast::VariableDecl>(currentDeclaration());
    assert(var_decl != nullptr);

    if (currentScope().isClass())
      processDataMemberDecl(var_decl);
    else
      throw std::runtime_error{ "Not implemented" };
    
  }
}



bool ScriptCompiler::compileFunctions()
{
  auto fcomp = std::shared_ptr<FunctionCompiler>(getComponent<FunctionCompiler>());

  for (size_t i(0); i < this->mCompilationTasks.size(); ++i)
  {
    const auto & task = this->mCompilationTasks.at(i);
    fcomp->compile(task);
  }

  return true;
}

static bool check_static_init(program::Expression &);

static bool check_static_init(const std::vector<std::shared_ptr<program::Expression>> & exprs)
{
  for (const auto & e : exprs)
  {
    if (!check_static_init(*e))
      return false;
  }

  return true;
}

class StaticInitializationChecker : public program::ExpressionVisitor
{
public:
  bool result;
public:
  StaticInitializationChecker() : result(true) { }
  ~StaticInitializationChecker() = default;

  Value visit(const program::ArrayExpression & ae)
  {
    result = check_static_init(ae.elements);
    return Value{};
  }

  Value visit(const program::BindExpression &)
  {
    result = false;
    return Value{};
  }

  Value visit(const program::CaptureAccess &)
  {
    result = false;
    return Value{};
  }

  Value visit(const program::CommaExpression &)
  {
    result = false;
    return Value{};
  }

  Value visit(const program::ConditionalExpression & ce)
  {
    result = check_static_init(*ce.cond) && check_static_init(*ce.onTrue) && check_static_init(*ce.onFalse);
    return Value{};
  }


  Value visit(const program::ConstructorCall & cc)
  {
    result = check_static_init(cc.arguments);
    return Value{};
  }

  Value visit(const program::Copy &)
  {
    result = true;
    return Value{};
  }

  Value visit(const program::FetchGlobal &)
  {
    result = false;
    return Value{};
  }

  Value visit(const program::FunctionCall & fc)
  {
    result = check_static_init(fc.args);
    return Value{};
  }

  Value visit(const program::FunctionVariableCall & fvc)
  {
    result = false;
    return Value{};
  }

  Value visit(const program::FundamentalConversion &)
  {
    result = true;
    return Value{};
  }

  Value visit(const program::LambdaExpression &)
  {
    result = false;
    return Value{};
  }

  Value visit(const program::Literal &)
  {
    result = true;
    return Value{};
  }

  Value visit(const program::LogicalAnd &)
  {
    result = true;
    return Value{};
  }

  Value visit(const program::LogicalOr &)
  {
    result = true;
    return Value{};
  }

  Value visit(const program::MemberAccess & ma)
  {
    result = check_static_init(*ma.object);
    return Value{};
  }

  Value visit(const program::StackValue &)
  {
    result = false;
    return Value{};
  }

  Value visit(const program::VirtualCall &)
  {
    result = false;
    return Value{};
  }

};

bool check_static_init(program::Expression & expr)
{
  StaticInitializationChecker checker;
  expr.accept(checker);
  return checker.result;
}


bool ScriptCompiler::checkStaticInitialization(const std::shared_ptr<program::Expression> & expr)
{
  return check_static_init(*expr);
}

bool ScriptCompiler::initializeStaticVariable(const StaticVariable & svar)
{
  auto exprcomp = std::unique_ptr<ExpressionCompiler>(getComponent<ExpressionCompiler>());
  exprcomp->setScope(svar.scope);

  if (svar.init == nullptr)
  {
    try
    {
      Value val = svar.variable;
      engine()->placement(val, {});
    }
    catch (...)
    {
      throw FailedToInitializeStaticVariable{};
    }
  }
  else
  {
    auto exprcomp = std::unique_ptr<ExpressionCompiler>(getComponent<ExpressionCompiler>());
    std::shared_ptr<program::Expression> initexpr = exprcomp->compile(svar.init, svar.scope);

    if (!checkStaticInitialization(initexpr))
      throw InvalidStaticInitialization{};

    if (initexpr->is<program::ConstructorCall>())
    {
      auto ctorcall = std::dynamic_pointer_cast<program::ConstructorCall>(initexpr);
      try
      {
        std::vector<Value> args;
        for (const auto & a : ctorcall->arguments)
        {
          args.push_back(engine()->implementation()->interpreter->eval(a));
        }
        Value val = svar.variable;
        engine()->placement(val, args);
      }
      catch (...)
      {
        throw FailedToInitializeStaticVariable{};
      }
    }
    else
    {
      try
      {
        Value val = svar.variable;
        Value arg = engine()->implementation()->interpreter->eval(initexpr);
        engine()->placement(val, { arg });
      }
      catch (...)
      {
        throw FailedToInitializeStaticVariable{};
      }
    }
  }

  return true;
}

bool ScriptCompiler::initializeStaticVariables()
{
  for (size_t i(0); i < mStaticVariables.size(); ++i)
  {
    initializeStaticVariable(mStaticVariables.at(i));
  }

  return true;
}


bool ScriptCompiler::isFirstOrderDeclaration(const std::shared_ptr<ast::Declaration> & decl) const
{
  if (decl->is<ast::ClassDecl>() || decl->is<ast::EnumDeclaration>() || decl->is<ast::Typedef>())
    return true;

  /*
  if (decl->is<ast::TemplateDeclaration>())
  {
    const ast::TemplateDeclaration & t_decl = decl->as<ast::TemplateDeclaration>();
    if (t_decl->params.size() > 0)
      return true;

    if (t_decl->params.size() == 0)
      return t_decl->body->is<ast::ClassDecl>();
  }
  */

  return false;
}

bool ScriptCompiler::isSecondOrderDeclaration(const std::shared_ptr<ast::Declaration>& decl) const
{
  switch (decl->type())
  {
  case ast::NodeType::FunctionDeclaration:
  case ast::NodeType::ConstructorDeclaration:
  case ast::NodeType::DestructorDeclaration:
  case ast::NodeType::OperatorOverloadDeclaration:
  case ast::NodeType::CastDeclaration:
    return true;
  default:
    break;
  }

  return false;
}

bool ScriptCompiler::isThirdOrderDeclaration(const std::shared_ptr<ast::Declaration> & decl) const
{
  return decl->is<ast::VariableDecl>();
}


bool ScriptCompiler::processClassDeclaration()
{
  std::shared_ptr<ast::ClassDecl> class_decl = std::dynamic_pointer_cast<ast::ClassDecl>(currentDeclaration());
  assert(class_decl != nullptr);

  const std::string & class_name = class_decl->name->getName();

  Class parentClass;
  if (class_decl->parent != nullptr)
  {
    NameLookup lookup = NameLookup::resolve(class_decl->parent, currentScope());
    if (lookup.resultType() != NameLookup::TypeName || !lookup.typeResult().isObjectType())
      throw InvalidBaseClass{ dpos(class_decl->parent) };
    parentClass = engine()->getClass(lookup.typeResult());
    assert(!parentClass.isNull());
  }

  ClassBuilder builder{ class_decl->name->getName() };
  builder.setParent(parentClass);
  Class cla = build(builder);
  cla.implementation()->script = script().weakref();
  currentScope().impl()->add_class(cla);

  script::Scope classScope = script::Scope(cla, currentScope());
  for (size_t i(0); i < class_decl->content.size(); ++i)
  {
    /// TODO : handle access specifier
    if (class_decl->content.at(i)->is<ast::Declaration>())
      processOrCollectDeclaration(std::dynamic_pointer_cast<ast::Declaration>(class_decl->content.at(i)), classScope);
    else if (class_decl->content.at(i)->is<ast::AccessSpecifier>())
      log(diagnostic::warning() << "Access specifiers are ignored (not implemented yet)");
  }

  return true;
}

void ScriptCompiler::processEnumDeclaration()
{
  const ast::EnumDeclaration & enum_decl = currentDeclaration()->as<ast::EnumDeclaration>();

  Enum e = build(Enum{}, enum_decl.name->getName());
  e.implementation()->script = script().weakref();
  currentScope().impl()->add_enum(e);

  for (size_t i(0); i < enum_decl.values.size(); ++i)
  {
    if (enum_decl.values.at(i).value != nullptr)
      throw NotImplementedError{ "Enum value with initialization are not supported yet" };
    else
      e.addValue(enum_decl.values.at(i).name->getName());
  }
}

void ScriptCompiler::processTypedef()
{
  const ast::Typedef & tdef = currentDeclaration()->as<ast::Typedef>();

  const Type t = resolve(tdef.qualified_type);
  const std::string & name = tdef.name->getName();

  currentScope().impl()->add_typedef(Typedef{ name, t });
}

bool ScriptCompiler::processFirstOrderTemplateDeclaration()
{
  throw NotImplementedError{ dpos(currentDeclaration()), "Template declarations not supported yet" };
  return false;
}


bool ScriptCompiler::processFunctionDeclaration()
{
  std::shared_ptr<ast::FunctionDecl> fundecl = std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration());

  FunctionBuilder builder = FunctionBuilder::Function(fundecl->name->getName(), functionPrototype());
  if (fundecl->deleteKeyword.isValid())
    builder.setDeleted();
  if (fundecl->virtualKeyword.isValid())
  {
    if (!currentScope().isClass())
      throw InvalidUseOfVirtualKeyword{ dpos(fundecl->virtualKeyword) };

    builder.setVirtual();
    if (fundecl->virtualPure.isValid())
      builder.setPureVirtual();
  }
  Function function = build(builder);

  currentScope().impl()->add_function(function);

  if (function.isVirtual() && !fundecl->virtualKeyword.isValid())
    log(diagnostic::warning() << diagnostic::pos(fundecl->pos().line, fundecl->pos().col)
      << "Function overriding base virtual member declared without virtual or override specifier");

  if (!function.isDeleted() && !function.isPureVirtual())
    schedule(CompileFunctionTask{ function, std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration()), currentScope() });

  return true;
}

bool ScriptCompiler::processConstructorDeclaration()
{
  const auto & ctor_decl = currentDeclaration()->as<ast::ConstructorDecl>();

  Class current_class = currentScope().asClass();
  Prototype proto = functionPrototype();

  FunctionBuilder b = FunctionBuilder::Constructor(current_class, proto);
  if (ctor_decl.explicitKeyword.isValid())
    b.setExplicit();
  if (ctor_decl.deleteKeyword.isValid())
    b.setDeleted();
  if (ctor_decl.defaultKeyword.isValid())
    b.setDefaulted();
  Function ctor = build(b);

  /// TODO : be careful not to add the constructor twice
  // for now this is okay since the FunctionBuilder never adds anything to the class.
  current_class.implementation()->registerConstructor(ctor);

  if (!ctor.isDeleted())
    schedule(CompileFunctionTask{ ctor, std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration()), currentScope() });

  return true;
}

bool ScriptCompiler::processDestructorDeclaration()
{
  const auto & dtor_decl = currentDeclaration()->as<ast::DestructorDecl>();

  Class current_class = currentScope().asClass();

  FunctionBuilder b = FunctionBuilder::Destructor(current_class);
  if (dtor_decl.virtualKeyword.isValid())
    b.setVirtual();
  if (dtor_decl.deleteKeyword.isValid())
    b.setDeleted();
  if (dtor_decl.defaultKeyword.isValid())
    b.setDefaulted();

  if (!current_class.parent().isNull())
  {
    Function parent_dtor = current_class.parent().destructor();
    if (!parent_dtor.isNull() && parent_dtor.isVirtual())
      b.setVirtual();
  }

  /// TODO : check if a destructor already exists
  Function dtor = build(b);
  current_class.implementation()->destructor = dtor;
  
  /// TODO : not sure why would anyone want to delete a destructor ?
  if(!dtor.isDeleted())
    schedule(CompileFunctionTask{ dtor, std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration()), currentScope() });

  return true;
}

bool ScriptCompiler::processLiteralOperatorDecl()
{
  const ast::OperatorOverloadDecl & over_decl = currentDeclaration()->as<ast::OperatorOverloadDecl>();

  /// TODO : check that we are at namespace level !

  Prototype proto = functionPrototype();

  std::string suffix_name = over_decl.name->as<ast::LiteralOperatorName>().suffix_string();

  LiteralOperator function{ std::make_shared<LiteralOperatorImpl>(std::move(suffix_name), proto, engine()) };

  this->currentScope().impl()->add_literal_operator(function);

  if (!function.isDeleted())
    schedule(CompileFunctionTask{ function, std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration()), currentScope() });

  return true;
}

bool ScriptCompiler::processOperatorOverloadingDeclaration()
{
  const ast::OperatorOverloadDecl & over_decl = currentDeclaration()->as<ast::OperatorOverloadDecl>();

  if (over_decl.name->is<ast::LiteralOperatorName>())
    return processLiteralOperatorDecl();

  Prototype proto = functionPrototype();
  
  const bool is_member = this->currentScope().type() == script::Scope::ClassScope;
  
  auto arity = proto.argc() == 2 ? ast::OperatorName::BuiltInOpResol::BinaryOp : ast::OperatorName::UnaryOp;
  Operator::BuiltInOperator operation = ast::OperatorName::getOperatorId(over_decl.name->name, arity);
  if (operation == Operator::Null)
  {
    // operator++(int) and operator++() trick
    if ((over_decl.name->name == parser::Token::PlusPlus || over_decl.name->name == parser::Token::MinusMinus)
      && (proto.argc() == 2 && proto.argv(1) == Type::Int))
    {
      proto.popArgument();
      operation = over_decl.name->name == parser::Token::PlusPlus ? Operator::PostIncrementOperator : Operator::PostDecrementOperator;
    }
    else
      throw CouldNotResolveOperatorName{ dpos(over_decl) };
  }

  if (Operator::isBinary(operation) && proto.argc() != 2)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), std::to_string(2), std::to_string(proto.argc()) };
  else if (Operator::isUnary(operation) && proto.argc() != 1)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), std::to_string(1), std::to_string(proto.argc()) };

  
  if (Operator::onlyAsMember(operation) && !is_member)
    throw OpOverloadMustBeDeclaredAsMember{ dpos(over_decl) };

  FunctionBuilder builder = FunctionBuilder::Operator(operation, proto);
  if (over_decl.deleteKeyword.isValid())
    builder.setDeleted();
  Operator function = build(builder).toOperator();

  this->currentScope().impl()->add_operator(function);
  
  if(!function.isDeleted())
    schedule(CompileFunctionTask{ function, std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration()), currentScope() });

  return true;
}

bool ScriptCompiler::processCastOperatorDeclaration()
{
  const ast::CastDecl & cast_decl = currentDeclaration()->as<ast::CastDecl>();

  Prototype proto = functionPrototype();

  const bool is_member = this->currentScope().type() == script::Scope::ClassScope;
  assert(is_member); /// TODO : is this necessary (should be enforced by the parser)

  FunctionBuilder builder = FunctionBuilder::Cast(proto.argv(0), proto.returnType());
  if (cast_decl.explicitKeyword.isValid())
    builder.setExplicit();
  if (cast_decl.deleteKeyword.isValid())
    builder.setDeleted();

  Cast cast = build(builder).toCast();

  this->currentScope().impl()->add_cast(cast);
  
  if (!cast.isDeleted())
    schedule(CompileFunctionTask{ cast, std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration()) , currentScope() });

  return true;
}

bool ScriptCompiler::processSecondOrderTemplateDeclaration()
{
  throw NotImplementedError{ dpos(currentDeclaration()), "Function templates not supported yet" };
}


TemplateArgument ScriptCompiler::processTemplateArg(const std::shared_ptr<ast::Expression> & arg)
{
  throw NotImplementedError{ dpos(arg), "Template argument evaluation not implemented yet" };
}


Prototype ScriptCompiler::functionPrototype()
{
  auto fundecl = std::dynamic_pointer_cast<ast::FunctionDecl>(currentDeclaration());

  Prototype result;

  if (fundecl->is<ast::ConstructorDecl>())
    result.setReturnType(Type{ currentScope().asClass().id(), Type::ReferenceFlag | Type::ConstFlag });
  else if (fundecl->is<ast::DestructorDecl>())
    result.setReturnType(Type::Void);
  else
    result.setReturnType(resolve(fundecl->returnType));

  if (currentScope().type() == script::Scope::ClassScope && fundecl->staticKeyword == parser::Token::Invalid
     && fundecl->is<ast::ConstructorDecl>() == false)
  {
    Type thisType{ currentScope().asClass().id(), Type::ReferenceFlag | Type::ThisFlag };
    if (fundecl->constQualifier.isValid())
      thisType.setFlag(Type::ConstFlag);

    result.addArgument(thisType);
  }

  bool mustbe_defaulted = false;
  for (size_t i(0); i < fundecl->params.size(); ++i)
  {
    Type argtype = resolve(fundecl->params.at(i).type);
    if (fundecl->params.at(i).defaultValue != nullptr)
      argtype.setFlag(Type::OptionalFlag), mustbe_defaulted = true;
    else if (mustbe_defaulted)
      throw InvalidUseOfDefaultArgument{ dpos(fundecl->params.at(i).name) };
    result.addArgument(argtype);
  }

  return result;
}

void ScriptCompiler::setCurrentScope(script::Scope scope)
{
  mCurrentScope = scope;
}

void ScriptCompiler::setCurrentDeclaration(const std::shared_ptr<ast::Declaration> & decl)
{
  mCurrentDeclaration = decl;
}

void ScriptCompiler::setState(const script::Scope & scope, const std::shared_ptr<ast::Declaration> & decl)
{
  mCurrentScope = scope;
  mCurrentDeclaration = decl;
}

void ScriptCompiler::schedule(const CompileFunctionTask & task)
{
  mCompilationTasks.push_back(task);
}

Class ScriptCompiler::build(const ClassBuilder & builder)
{
  Class cla = CompilerComponent::build(builder);
  cla.implementation()->script = mScript.weakref();
  return cla;
}

Function ScriptCompiler::build(const FunctionBuilder & builder)
{
  Function f = CompilerComponent::build(builder);
  f.implementation()->script = mScript.weakref();
  return f;
}

Enum ScriptCompiler::build(const Enum &, const std::string & name)
{
  Enum e = CompilerComponent::build(Enum{}, name);
  e.implementation()->script = mScript.weakref();
  return e;
}

} // namespace compiler

} // namespace script

