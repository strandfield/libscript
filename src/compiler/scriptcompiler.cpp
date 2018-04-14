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
#include "../namespace_p.h"
#include "../scope_p.h"
#include "../script_p.h"

namespace script
{

namespace compiler
{

ScriptCompiler::ScriptCompiler(Compiler *c, CompileSession *s)
  : CompilerComponent(c, s)
  , mExprCompiler(c, s)
{

}

void ScriptCompiler::compile(const CompileScriptTask & task)
{
  mScript = task.script;
  mAst = task.ast;
  mCurrentScope = Scope{ mScript, Scope{engine()->rootNamespace()} };

  assert(mAst != nullptr);

  Function program = registerRootFunction(mCurrentScope);
  processFirstOrderDeclarationsAndCollectHigherOrderDeclarations();
  processSecondOrderDeclarations();
  processThirdOrderDeclarations();
  compileFunctions();
  initializeStaticVariables();

  mScript.implementation()->program = program;
}

std::string ScriptCompiler::repr(const std::shared_ptr<ast::Identifier> & id)
{
  return mExprCompiler.repr(id);
}

Type ScriptCompiler::resolve(const ast::QualifiedType & qt, const Scope & scp)
{
  mExprCompiler.setScope(scp);
  return mExprCompiler.resolve(qt);
}

NameLookup ScriptCompiler::resolve(const std::shared_ptr<ast::Identifier> & id, const Scope & scp)
{
  mExprCompiler.setScope(scp);
  return mExprCompiler.resolve(id);
}

Function ScriptCompiler::registerRootFunction(const Scope & scp)
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
    case ast::NodeType::NamespaceDecl:
      break;
    case ast::NodeType::CastDeclaration:
      throw NotImplementedError{ "ScriptCompiler::registerRootFunction() : cast declaration are not allowed at this scope" };
    default:
      fakedecl->body->statements.push_back(s);
      break;
    }
  }

  schedule(CompileFunctionTask{ Function{scriptfunc}, fakedecl, scp });

  return scriptfunc;
}

bool ScriptCompiler::processFirstOrderDeclarationsAndCollectHigherOrderDeclarations()
{
  for (const auto & decl : mAst->declarations())
    processOrCollectDeclaration(decl, mCurrentScope);

  return true;
}

void ScriptCompiler::processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const Scope & scope)
{
  if (isFirstOrderDeclaration(declaration))
  {
    switch (declaration->type())
    {
    case ast::NodeType::ClassDeclaration:
      processClassDeclaration(std::static_pointer_cast<ast::ClassDecl>(declaration), scope);
      break;
    case ast::NodeType::EnumDeclaration:
      processEnumDeclaration(std::static_pointer_cast<ast::EnumDeclaration>(declaration), scope);
      break;
    case ast::NodeType::Typedef:
      processTypedef(std::static_pointer_cast<ast::Typedef>(declaration), scope);
      break;
    case ast::NodeType::NamespaceDecl:
      processNamespaceDecl(std::static_pointer_cast<ast::NamespaceDeclaration>(declaration), scope);
      break;
      //case ast::NodeType::TemplateDeclaration:
      //  processFirstOrderTemplateDeclaration();
      //  break;
    default:
      throw NotImplementedError{"This kind of declaration is not implemented yet"};
    }
  }
  else if (isSecondOrderDeclaration(declaration))
    mSecondOrderDeclarations.push_back(ScopedDeclaration{ scope, declaration });
  else if (isThirdOrderDeclaration(declaration))
    mThirdOrderDeclarations.push_back(ScopedDeclaration{ scope, declaration });
  else
    throw NotImplementedError{ "This kind of declaration is not implemented yet" };

  // the lock will restore the previous state 
  // thanks RAII !
}


void ScriptCompiler::processSecondOrderDeclarations()
{
  for (const auto & decl : mSecondOrderDeclarations)
    processSecondOrderDeclaration(decl);
}


void ScriptCompiler::processSecondOrderDeclaration(const compiler::ScopedDeclaration & decl)
{
  switch (decl.declaration->type())
  {
  case ast::NodeType::FunctionDeclaration:
    return processFunctionDeclaration(std::static_pointer_cast<ast::FunctionDecl>(decl.declaration), decl.scope);
  case ast::NodeType::ConstructorDeclaration:
    return processConstructorDeclaration(std::static_pointer_cast<ast::ConstructorDecl>(decl.declaration), decl.scope);
  case ast::NodeType::DestructorDeclaration:
    return processDestructorDeclaration(std::static_pointer_cast<ast::DestructorDecl>(decl.declaration), decl.scope);
  case ast::NodeType::OperatorOverloadDeclaration:
    return processOperatorOverloadingDeclaration(std::static_pointer_cast<ast::OperatorOverloadDecl>(decl.declaration), decl.scope);
  case ast::NodeType::CastDeclaration:
    return processCastOperatorDeclaration(std::static_pointer_cast<ast::CastDecl>(decl.declaration), decl.scope);
    //case ast::NodeType::TemplateDeclaration:
    //  return processSecondOrderTemplateDeclaration();
  default:
    break;
  }

  throw NotImplementedError{"ScriptCompiler::processSecondOrderDeclaration() : implementation error"};
}

void ScriptCompiler::processDataMemberDecl(const std::shared_ptr<ast::VariableDecl> & var_decl, const Scope & scp)
{
  Class current_class = scp.asClass();
  assert(!current_class.isNull());

  if (var_decl->variable_type.type->name == parser::Token::Auto)
    throw DataMemberCannotBeAuto{ dpos(var_decl) };

  /// TODO : consider variable which are functions
  if (var_decl->variable_type.functionType != nullptr)
    throw NotImplementedError{ "Static variables of function-type are not implemented yet" };

  /// TODO : consider const-ref qualifications 
  NameLookup lookup = NameLookup::resolve(var_decl->variable_type.type, scp);
  if (lookup.resultType() != NameLookup::TypeName)
    throw InvalidTypeName{ dpos(var_decl), repr(var_decl->variable_type.type) };

  if (var_decl->staticSpecifier.isValid())
  {
    if (var_decl->init == nullptr)
      throw MissingStaticInitialization{ dpos(var_decl) };

    if (var_decl->init->is<ast::ConstructorInitialization>() || var_decl->init->is<ast::BraceInitialization>())
      throw InvalidStaticInitialization{ dpos(var_decl) };

    auto expr = var_decl->init->as<ast::AssignmentInitialization>().value;
    if (lookup.typeResult().isFundamentalType() && expr->is<ast::Literal>() && !expr->is<ast::UserDefinedLiteral>())
    {
      auto execexpr = mExprCompiler.compile(expr, scp);
      
      Value val = engine()->implementation()->interpreter->eval(execexpr);
      current_class.addStaticDataMember(var_decl->name->getName(), val, getAccessSpecifier(scp));
    }
    else
    {
      Value staticMember = current_class.implementation()->add_uninitialized_static_data_member(var_decl->name->getName(), lookup.typeResult(), getAccessSpecifier(scp));
      mStaticVariables.push_back(StaticVariable{ staticMember, var_decl, scp });
    }
  }
  else
  {
    Class::DataMember dataMember{ lookup.typeResult(), var_decl->name->getName(), getAccessSpecifier(scp) };
    current_class.implementation()->dataMembers.push_back(dataMember);
  }
}

void ScriptCompiler::processNamespaceVariableDecl(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp)
{
  Namespace ns = scp.asNamespace();
  assert(!ns.isNull());

  if (decl->variable_type.type->name == parser::Token::Auto)
    throw GlobalVariablesCannotBeAuto{ dpos(decl) };

  /// TODO : consider variable which are functions
  if (decl->variable_type.functionType != nullptr)
    throw NotImplementedError{ "Static variables of function-type are not implemented yet" };

  /// TODO : consider const-ref qualifications 
  NameLookup lookup = NameLookup::resolve(decl->variable_type.type, scp);
  if (lookup.resultType() != NameLookup::TypeName)
    throw InvalidTypeName{ dpos(decl), repr(decl->variable_type.type) };

  if (decl->init == nullptr)
    throw GlobalVariablesMustBeInitialized{ dpos(decl) };

  if (decl->init->is<ast::ConstructorInitialization>() || decl->init->is<ast::BraceInitialization>())
    throw GlobalVariablesMustBeAssigned{ dpos(decl) };

  auto expr = decl->init->as<ast::AssignmentInitialization>().value;
  Value val;
  if (lookup.typeResult().isFundamentalType() && expr->is<ast::Literal>() && !expr->is<ast::UserDefinedLiteral>())
  {
    auto execexpr = mExprCompiler.compile(expr, scp);
    val = engine()->implementation()->interpreter->eval(execexpr);
  }
  else
  {
    val = engine()->uninitialized(lookup.typeResult());
    mStaticVariables.push_back(StaticVariable{ val, decl, scp });
  }

  engine()->manage(val);
  ns.implementation()->variables[decl->name->getName()] = val;
}

void ScriptCompiler::processFriendDecl(const std::shared_ptr<ast::FriendDeclaration> & decl, const Scope & scp)
{
  assert(decl->is<ast::ClassFriendDeclaration>());

  const auto & pal = decl->as<ast::ClassFriendDeclaration>();

  NameLookup lookup = resolve(pal.class_name, scp);
  if (lookup.typeResult().isNull())
    throw InvalidTypeName{ dpos(pal.class_name), dstr(pal.class_name) };

  if (!lookup.typeResult().isObjectType())
    throw FriendMustBeAClass{ dpos(pal.class_name) };

  scp.asClass().addFriend(engine()->getClass(lookup.typeResult()));
}

void ScriptCompiler::processThirdOrderDeclarations()
{
  for (size_t i(0); i < mThirdOrderDeclarations.size(); ++i)
  {
    const auto & decl = mThirdOrderDeclarations.at(i);

    if (decl.declaration->is<ast::FriendDeclaration>())
    {
      processFriendDecl(std::static_pointer_cast<ast::FriendDeclaration>(decl.declaration), decl.scope);
    }
    else
    {
      assert(decl.declaration->is<ast::VariableDecl>());

      if (decl.scope.type() == Scope::ScriptScope)
      {
        // Global variables are processed by the function compiler
        continue;
      }

      std::shared_ptr<ast::VariableDecl> var_decl = std::dynamic_pointer_cast<ast::VariableDecl>(decl.declaration);
      assert(var_decl != nullptr);

      if (decl.scope.isClass())
        processDataMemberDecl(var_decl, decl.scope);
      else if (decl.scope.type() == Scope::NamespaceScope)
        processNamespaceVariableDecl(var_decl, decl.scope);
      else
        throw std::runtime_error{ "Not implemented" };
    }
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
  mExprCompiler.setScope(svar.scope);

  const auto & init = svar.declaration->init;
  auto parsed_initexpr = init->as<ast::AssignmentInitialization>().value;
  if (parsed_initexpr == nullptr)
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
    std::shared_ptr<program::Expression> initexpr = mExprCompiler.compile(parsed_initexpr, svar.scope);

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
  if (decl->is<ast::ClassDecl>() || decl->is<ast::EnumDeclaration>() || decl->is<ast::Typedef>() || decl->is<ast::NamespaceDeclaration>())
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
  return decl->is<ast::VariableDecl>() || decl->is<ast::FriendDeclaration>();
}


static inline AccessSpecifier get_access_specifier(const std::shared_ptr<ast::AccessSpecifier> & as)
{
  if (as->visibility == parser::Token::Private)
    return AccessSpecifier::Private;
  else if (as->visibility == parser::Token::Protected)
    return AccessSpecifier::Protected;
  return AccessSpecifier::Public;
}

void ScriptCompiler::processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & class_decl, const Scope & scp)
{
  assert(class_decl != nullptr);

  const std::string & class_name = class_decl->name->getName();

  Class parent_class;
  if (class_decl->parent != nullptr)
  {
    NameLookup lookup = NameLookup::resolve(class_decl->parent, scp);
    if (lookup.resultType() != NameLookup::TypeName || !lookup.typeResult().isObjectType())
      throw InvalidBaseClass{ dpos(class_decl->parent) };
    parent_class = engine()->getClass(lookup.typeResult());
    assert(!parent_class.isNull());
  }

  ClassBuilder builder{ class_decl->name->getName() };
  builder.setParent(parent_class);
  Class cla = build(builder);
  cla.implementation()->script = script().weakref();
  scp.impl()->add_class(cla);

  Scope class_scope = Scope{ cla, scp };
  for (size_t i(0); i < class_decl->content.size(); ++i)
  {
    if (class_decl->content.at(i)->is<ast::Declaration>())
      processOrCollectDeclaration(std::dynamic_pointer_cast<ast::Declaration>(class_decl->content.at(i)), class_scope);
    else if (class_decl->content.at(i)->is<ast::AccessSpecifier>())
    {
      log(diagnostic::warning() << "Access specifiers are ignored for type members");
      AccessSpecifier aspec = get_access_specifier(std::dynamic_pointer_cast<ast::AccessSpecifier>(class_decl->content.at(i)));
      class_scope = Scope{ std::static_pointer_cast<ClassScope>(class_scope.impl())->withAccessibility(aspec) };
    }
  }
}

void ScriptCompiler::processEnumDeclaration(const std::shared_ptr<ast::EnumDeclaration> & decl, const Scope & scp)
{
  const ast::EnumDeclaration & enum_decl = *decl;

  Enum e = build(Enum{}, enum_decl.name->getName());
  e.implementation()->script = script().weakref();
  scp.impl()->add_enum(e);

  for (size_t i(0); i < enum_decl.values.size(); ++i)
  {
    if (enum_decl.values.at(i).value != nullptr)
      throw NotImplementedError{ "Enum value with initialization are not supported yet" };
    else
      e.addValue(enum_decl.values.at(i).name->getName());
  }
}

void ScriptCompiler::processTypedef(const std::shared_ptr<ast::Typedef> & decl, const Scope & scp)
{
  const ast::Typedef & tdef = *decl;

  const Type t = resolve(tdef.qualified_type, scp);
  const std::string & name = tdef.name->getName();

  scp.impl()->add_typedef(Typedef{ name, t });
}

void ScriptCompiler::processNamespaceDecl(const std::shared_ptr<ast::NamespaceDeclaration> & decl, const Scope & scp)
{
  const ast::NamespaceDeclaration & ndecl = *decl;

  if (scp.type() != Scope::NamespaceScope && scp.type() != Scope::ScriptScope)
    throw NamespaceDeclarationCannotAppearAtThisLevel{ dpos(decl) };

  Namespace parent_ns = scp.type() == Scope::NamespaceScope ? scp.asNamespace() : scp.asScript().rootNamespace();
  const std::string name = ndecl.namespace_name->getName();

  Namespace ns = parent_ns.newNamespace(name); /// TODO : what-if the namespace already exists ?

  Scope child_scope = scp.child(name);
  for (const auto & s : ndecl.statements)
  {
    if (!s->is<ast::Declaration>())
      throw ExpectedDeclaration{ dpos(s) };

    processOrCollectDeclaration(std::static_pointer_cast<ast::Declaration>(s), child_scope);
  }
}

void ScriptCompiler::processFirstOrderTemplateDeclaration(const std::shared_ptr<ast::Declaration> & decl, const Scope &)
{
  throw NotImplementedError{ dpos(decl), "Template declarations not supported yet" };
}

AccessSpecifier ScriptCompiler::getAccessSpecifier(const Scope & scp)
{
  if (!scp.isClass())
    return AccessSpecifier::Public;

  return std::static_pointer_cast<ClassScope>(scp.impl())->mAccessibility;
}

void ScriptCompiler::handleAccessSpecifier(FunctionBuilder &builder, const Scope & scp)
{
  switch (getAccessSpecifier(scp))
  {
  case AccessSpecifier::Protected:
    builder.setProtected();
    break;
  case AccessSpecifier::Private:
    builder.setPrivate();
    break;
  default:
    break;
  }
}

void ScriptCompiler::processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
{
  FunctionBuilder builder = FunctionBuilder::Function(fundecl->name->getName(), functionPrototype(fundecl, scp));
  if (fundecl->deleteKeyword.isValid())
    builder.setDeleted();
  if (fundecl->virtualKeyword.isValid())
  {
    if (!scp.isClass())
      throw InvalidUseOfVirtualKeyword{ dpos(fundecl->virtualKeyword) };

    builder.setVirtual();
    if (fundecl->virtualPure.isValid())
      builder.setPureVirtual();
  }
  handleAccessSpecifier(builder, scp);
  Function function = build(builder);

  scp.impl()->add_function(function);

  if (function.isVirtual() && !fundecl->virtualKeyword.isValid())
    log(diagnostic::warning() << diagnostic::pos(fundecl->pos().line, fundecl->pos().col)
      << "Function overriding base virtual member declared without virtual or override specifier");

  if (!function.isDeleted() && !function.isPureVirtual())
    schedule(CompileFunctionTask{ function, fundecl, scp });
}

void ScriptCompiler::processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl, const Scope & scp)
{
  const auto & ctor_decl = *decl;
  Class current_class = scp.asClass();

  Prototype proto = functionPrototype(decl, scp);

  FunctionBuilder b = FunctionBuilder::Constructor(current_class, proto);
  if (ctor_decl.explicitKeyword.isValid())
    b.setExplicit();
  if (ctor_decl.deleteKeyword.isValid())
    b.setDeleted();
  if (ctor_decl.defaultKeyword.isValid())
    b.setDefaulted();
  handleAccessSpecifier(b, scp);
  Function ctor = build(b);

  /// TODO : be careful not to add the constructor twice
  // for now this is okay since the FunctionBuilder never adds anything to the class.
  current_class.implementation()->registerConstructor(ctor);

  if (!ctor.isDeleted())
    schedule(CompileFunctionTask{ ctor, decl, scp });
}

void ScriptCompiler::processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl, const Scope & scp)
{
  const auto & dtor_decl = *decl;
  Class current_class = scp.asClass();

  FunctionBuilder b = FunctionBuilder::Destructor(current_class);
  if (dtor_decl.virtualKeyword.isValid())
    b.setVirtual();
  if (dtor_decl.deleteKeyword.isValid())
    b.setDeleted();
  if (dtor_decl.defaultKeyword.isValid())
    b.setDefaulted();

  handleAccessSpecifier(b, scp);

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
    schedule(CompileFunctionTask{ dtor, decl, scp });
}

void ScriptCompiler::processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl, const Scope & scp)
{
  const ast::OperatorOverloadDecl & over_decl = *decl;

  /// TODO : check that we are at namespace level !

  Prototype proto = functionPrototype(decl, scp);

  std::string suffix_name = over_decl.name->as<ast::LiteralOperatorName>().suffix_string();

  LiteralOperator function{ std::make_shared<LiteralOperatorImpl>(std::move(suffix_name), proto, engine()) };

  scp.impl()->add_literal_operator(function);

  if (!function.isDeleted())
    schedule(CompileFunctionTask{ function, decl, scp });
}

void ScriptCompiler::processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl, const Scope & scp)
{
  const ast::OperatorOverloadDecl & over_decl = *decl;

  if (over_decl.name->is<ast::LiteralOperatorName>())
    return processLiteralOperatorDecl(decl, scp);

  Prototype proto = functionPrototype(decl, scp);
  
  const bool is_member = scp.type() == script::Scope::ClassScope;
  
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

  handleAccessSpecifier(builder, scp);

  Operator function = build(builder).toOperator();

  scp.impl()->add_operator(function);
  
  if(!function.isDeleted())
    schedule(CompileFunctionTask{ function, decl, scp });
}

void ScriptCompiler::processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl, const Scope & scp)
{
  const ast::CastDecl & cast_decl = *decl;

  Prototype proto = functionPrototype(decl, scp);

  const bool is_member = scp.type() == script::Scope::ClassScope;
  assert(is_member); /// TODO : is this necessary (should be enforced by the parser)

  FunctionBuilder builder = FunctionBuilder::Cast(proto.argv(0), proto.returnType());
  if (cast_decl.explicitKeyword.isValid())
    builder.setExplicit();
  if (cast_decl.deleteKeyword.isValid())
    builder.setDeleted();

  handleAccessSpecifier(builder, scp);

  Cast cast = build(builder).toCast();

  scp.impl()->add_cast(cast);
  
  if (!cast.isDeleted())
    schedule(CompileFunctionTask{ cast, decl, scp });
}

void ScriptCompiler::processSecondOrderTemplateDeclaration(const std::shared_ptr<ast::Declaration> & decl, const Scope &)
{
  throw NotImplementedError{ dpos(decl), "Function templates not supported yet" };
}


TemplateArgument ScriptCompiler::processTemplateArg(const std::shared_ptr<ast::Expression> & arg)
{
  throw NotImplementedError{ dpos(arg), "Template argument evaluation not implemented yet" };
}


Prototype ScriptCompiler::functionPrototype(const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
{
  Prototype result;

  if (fundecl->is<ast::ConstructorDecl>())
    result.setReturnType(Type{ scp.asClass().id(), Type::ReferenceFlag | Type::ConstFlag });
  else if (fundecl->is<ast::DestructorDecl>())
    result.setReturnType(Type::Void);
  else
    result.setReturnType(resolve(fundecl->returnType, scp));

  if (scp.type() == script::Scope::ClassScope && fundecl->staticKeyword == parser::Token::Invalid
     && fundecl->is<ast::ConstructorDecl>() == false)
  {
    Type thisType{ scp.asClass().id(), Type::ReferenceFlag | Type::ThisFlag };
    if (fundecl->constQualifier.isValid())
      thisType.setFlag(Type::ConstFlag);

    result.addArgument(thisType);
  }

  bool mustbe_defaulted = false;
  for (size_t i(0); i < fundecl->params.size(); ++i)
  {
    Type argtype = resolve(fundecl->params.at(i).type, scp);
    if (fundecl->params.at(i).defaultValue != nullptr)
      argtype.setFlag(Type::OptionalFlag), mustbe_defaulted = true;
    else if (mustbe_defaulted)
      throw InvalidUseOfDefaultArgument{ dpos(fundecl->params.at(i).name) };
    result.addArgument(argtype);
  }

  return result;
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

