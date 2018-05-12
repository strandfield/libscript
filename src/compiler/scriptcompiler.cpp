// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/scriptcompiler.h"

#include "script/compiler/compilererrors.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/templatedefinition.h"

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/parser/parser.h"

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
#include "script/private/scope_p.h"
#include "../script_p.h"
#include "../template_p.h"

namespace script
{

namespace compiler
{

static inline AccessSpecifier get_access_specifier(const std::shared_ptr<ast::AccessSpecifier> & as)
{
  if (as->visibility == parser::Token::Private)
    return AccessSpecifier::Private;
  else if (as->visibility == parser::Token::Protected)
    return AccessSpecifier::Protected;
  return AccessSpecifier::Public;
}

ScriptCompiler::StateGuard::StateGuard(ScriptCompiler *c)
  : compiler(c)
  , script(c->mCurrentScript)
  , scope(c->mCurrentScope)
  , ast(c->mCurrentAst)
{

}

ScriptCompiler::StateGuard::~StateGuard()
{
  compiler->mCurrentAst = ast;
  compiler->mCurrentScope = scope;
  compiler->mCurrentScript = script;
}

Engine* ScriptCompilerModuleLoader::engine() const
{
  return compiler_->engine();
}

Script ScriptCompilerModuleLoader::load(const SourceFile &src)
{
  Script s = engine()->newScript(src);

  parser::Parser parser{ s.source() };
  auto ast = parser.parse(s.source());

  assert(ast != nullptr);

  compiler_->addTask(CompileScriptTask{ s, ast });

  return s;
}

ScriptCompiler::ScriptCompiler(Compiler *c, CompileSession *s)
  : CompilerComponent(c, s)
  , variable_(c->engine())
{
  name_resolver.compiler = this;
  type_resolver.name_resolver() = name_resolver;

  function_processor_.prototype_.type_.name_resolver() = name_resolver;

  scope_statements_.scope_ = &mCurrentScope;

  modules_.loader_.compiler_ = this;
}

void ScriptCompiler::compile(const CompileScriptTask & task)
{
  mTasks.clear();
  mTasks.push_back(task);

  processOrCollectScriptDeclarations(task);

  resolveIncompleteTypes();
  processPendingDeclarations();
  compileFunctions();
  //initializeStaticVariables();
  variable_.initializeVariables();

  for (size_t i(1); i < mTasks.size(); ++i)
  {
    mTasks.at(i).script.run();
  }
}

Class ScriptCompiler::compileClassTemplate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args, const std::shared_ptr<ast::ClassDecl> & class_decl)
{
  ClassBuilder builder{ std::string{} };
  fill(builder, class_decl);

  Class result{ ClassTemplateInstance::make(builder, ct, args) };
  result.implementation()->script = script().weakref();
  
  StateGuard guard{ this };
  mCurrentScope = ct.argumentScope(args);

  readClassContent(result, class_decl);

  // This is a standalone job 
  resolveIncompleteTypes();
  processPendingDeclarations();
  compileFunctions();
  variable_.initializeVariables();

  for (size_t i(1); i < mTasks.size(); ++i)
  {
    mTasks.at(i).script.run();
  }

  return result;
}

void ScriptCompiler::addTask(const CompileScriptTask & task)
{
  if (task.ast->hasErrors())
  {
    log(diagnostic::info() << "While loading script module:");
    for (const auto & m : task.ast->messages())
      log(m);

    /// TODO : destroy scripts
    return;
  }


  mTasks.push_back(task);
  processOrCollectScriptDeclarations(mTasks.back());
}

Class ScriptCompiler::addTask(const ClassTemplate & ct, const std::vector<TemplateArgument> & args, const std::shared_ptr<ast::ClassDecl> & class_decl)
{
  ClassBuilder builder{ std::string{} };
  fill(builder, class_decl);

  Class result{ ClassTemplateInstance::make(builder, ct, args) };
  result.implementation()->script = script().weakref();
  
  StateGuard guard{ this };
  mCurrentScope = ct.argumentScope(args);

  readClassContent(result, class_decl);

  return result;
}

Type ScriptCompiler::resolve(const ast::QualifiedType & qt)
{
  return type_resolver.resolve(qt, mCurrentScope);
}

NameLookup ScriptCompiler::resolve(const std::shared_ptr<ast::Identifier> & id)
{
  return name_resolver.resolve(id, mCurrentScope);
}

Function ScriptCompiler::registerRootFunction()
{
  auto scriptfunc = std::make_shared<ScriptFunctionImpl>(engine());
  scriptfunc->script = mCurrentScript.weakref();
  
  auto fakedecl = ast::FunctionDecl::New(std::shared_ptr<ast::AST>());
  fakedecl->body = ast::CompoundStatement::New(parser::Token{ parser::Token::LeftBrace, 0, 0, 0, 0 }, parser::Token{ parser::Token::RightBrace, 0, 0, 0, 0 });
  const auto & stmts = mCurrentAst->statements();
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
    case ast::NodeType::TemplateDecl:
      break;
    case ast::NodeType::CastDeclaration:
      throw NotImplementedError{ "ScriptCompiler::registerRootFunction() : cast declaration are not allowed at this scope" };
    default:
      fakedecl->body->statements.push_back(s);
      break;
    }
  }

  schedule(Function{ scriptfunc }, fakedecl, mCurrentScope);

  return Function{ scriptfunc };
}

void ScriptCompiler::processOrCollectScriptDeclarations(const CompileScriptTask & task)
{
  StateGuard guard{ this };

  mCurrentAst = task.ast;
  mCurrentScript = task.script;

  mCurrentScope = Scope{ mCurrentScript.rootNamespace() };
  mCurrentScope.merge(engine()->rootNamespace());
  mCurrentScope = Scope{ mCurrentScript, mCurrentScope };

  Function program = registerRootFunction();
  mCurrentScript.implementation()->program = program;
  processOrCollectScriptDeclarations();
}

bool ScriptCompiler::processOrCollectScriptDeclarations()
{
  for (const auto & decl : mCurrentAst->declarations())
    processOrCollectDeclaration(decl);

  return true;
}

void ScriptCompiler::processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration, const Scope & scp)
{
  ScopeGuard guard{ mCurrentScope };
  mCurrentScope = scp;

  processOrCollectDeclaration(declaration);
}

void ScriptCompiler::processOrCollectDeclaration(const std::shared_ptr<ast::Declaration> & declaration)
{
  switch (declaration->type())
  {
  case ast::NodeType::ClassDeclaration:
    processClassDeclaration(std::static_pointer_cast<ast::ClassDecl>(declaration));
    break;
  case ast::NodeType::EnumDeclaration:
    processEnumDeclaration(std::static_pointer_cast<ast::EnumDeclaration>(declaration));
    break;
  case ast::NodeType::Typedef:
    processTypedef(std::static_pointer_cast<ast::Typedef>(declaration));
    break;
  case ast::NodeType::NamespaceDecl:
    processNamespaceDecl(std::static_pointer_cast<ast::NamespaceDeclaration>(declaration));
    break;
  case ast::NodeType::UsingDirective:
  case ast::NodeType::UsingDeclaration:
  case ast::NodeType::NamespaceAliasDef:
  case ast::NodeType::TypeAliasDecl:
    return scope_statements_.process(declaration);
  case ast::NodeType::ImportDirective:
    processImportDirective(std::static_pointer_cast<ast::ImportDirective>(declaration));
    break;
  case ast::NodeType::FunctionDeclaration:
    return processFunctionDeclaration(std::static_pointer_cast<ast::FunctionDecl>(declaration));
  case ast::NodeType::ConstructorDeclaration:
    return processConstructorDeclaration(std::static_pointer_cast<ast::ConstructorDecl>(declaration));
  case ast::NodeType::DestructorDeclaration:
    return processDestructorDeclaration(std::static_pointer_cast<ast::DestructorDecl>(declaration));
  case ast::NodeType::OperatorOverloadDeclaration:
    return processOperatorOverloadingDeclaration(std::static_pointer_cast<ast::OperatorOverloadDecl>(declaration));
  case ast::NodeType::CastDeclaration:
    return processCastOperatorDeclaration(std::static_pointer_cast<ast::CastDecl>(declaration));
  case ast::NodeType::VariableDeclaration:
  case ast::NodeType::ClassFriendDecl:
    return collectDeclaration(declaration);
  case ast::NodeType::TemplateDecl:
    return processTemplateDeclaration(std::static_pointer_cast<ast::TemplateDeclaration>(declaration));
  default:
    throw NotImplementedError{ "This kind of declaration is not implemented yet" };
  }
}

void ScriptCompiler::collectDeclaration(const std::shared_ptr<ast::Declaration> & decl)
{
  mProcessingQueue.push_back(ScopedDeclaration{ currentScope(), decl });
}

void ScriptCompiler::resolveIncompleteTypes()
{
  ScopeGuard guard{ mCurrentScope };

  for (const auto & f : mIncompleteFunctions)
    reprocess(f);

  mIncompleteFunctions.clear();
}

void ScriptCompiler::processDataMemberDecl(const std::shared_ptr<ast::VariableDecl> & var_decl)
{
  const Scope & scp = currentScope();
  Class current_class = scp.asClass();
  assert(!current_class.isNull());

  if (var_decl->variable_type.type->name == parser::Token::Auto)
    throw DataMemberCannotBeAuto{ dpos(var_decl) };

  Type var_type = resolve(var_decl->variable_type);

  if (var_decl->staticSpecifier.isValid())
  {
    if (var_decl->init == nullptr)
      throw MissingStaticInitialization{ dpos(var_decl) };

    if (var_decl->init->is<ast::ConstructorInitialization>() || var_decl->init->is<ast::BraceInitialization>())
      throw InvalidStaticInitialization{ dpos(var_decl) };

    auto expr = var_decl->init->as<ast::AssignmentInitialization>().value;
    if (var_type.isFundamentalType() && expr->is<ast::Literal>() && !expr->is<ast::UserDefinedLiteral>())
    {
      auto execexpr = generateExpression(expr);
      
      Value val = engine()->implementation()->interpreter->eval(execexpr);
      current_class.addStaticDataMember(var_decl->name->getName(), val, scp.accessibility());
    }
    else
    {
      Value staticMember = current_class.implementation()->add_uninitialized_static_data_member(var_decl->name->getName(), var_type, scp.accessibility());
      mStaticVariables.push_back(StaticVariable{ staticMember, var_decl, scp });
    }
  }
  else
  {
    Class::DataMember dataMember{ var_type, var_decl->name->getName(), scp.accessibility() };
    current_class.implementation()->dataMembers.push_back(dataMember);
  }
}

void ScriptCompiler::processNamespaceVariableDecl(const std::shared_ptr<ast::VariableDecl> & decl)
{
  const Scope & scp = currentScope();
  Namespace ns = scp.asNamespace();
  assert(!ns.isNull());

  if (decl->variable_type.type->name == parser::Token::Auto)
    throw GlobalVariablesCannotBeAuto{ dpos(decl) };

  Type var_type = resolve(decl->variable_type);

  if (decl->init == nullptr)
    throw GlobalVariablesMustBeInitialized{ dpos(decl) };

  if (decl->init->is<ast::ConstructorInitialization>() || decl->init->is<ast::BraceInitialization>())
    throw GlobalVariablesMustBeAssigned{ dpos(decl) };

  auto expr = decl->init->as<ast::AssignmentInitialization>().value;
  Value val;
  if (var_type.isFundamentalType() && expr->is<ast::Literal>() && !expr->is<ast::UserDefinedLiteral>())
  {
    auto execexpr = generateExpression(expr);
    val = engine()->implementation()->interpreter->eval(execexpr);
  }
  else
  {
    val = engine()->uninitialized(var_type);
    mStaticVariables.push_back(StaticVariable{ val, decl, scp });
  }

  engine()->manage(val);
  ns.implementation()->variables[decl->name->getName()] = val;
}

void ScriptCompiler::processFriendDecl(const std::shared_ptr<ast::FriendDeclaration> & decl)
{
  assert(decl->is<ast::ClassFriendDeclaration>());

  const auto & pal = decl->as<ast::ClassFriendDeclaration>();

  NameLookup lookup = resolve(pal.class_name);
  if (lookup.typeResult().isNull())
    throw InvalidTypeName{ dpos(pal.class_name), dstr(pal.class_name) };

  if (!lookup.typeResult().isObjectType())
    throw FriendMustBeAClass{ dpos(pal.class_name) };

  currentScope().asClass().addFriend(engine()->getClass(lookup.typeResult()));
}

void ScriptCompiler::processPendingDeclarations()
{
  for (size_t i(0); i < mProcessingQueue.size(); ++i)
  {
    const auto & decl = mProcessingQueue.at(i);

    ScopeGuard guard{ mCurrentScope };
    mCurrentScope = decl.scope;

    if (decl.declaration->is<ast::FriendDeclaration>())
    {
      processFriendDecl(std::static_pointer_cast<ast::FriendDeclaration>(decl.declaration));
    }
    else
    {
      variable_.process(std::static_pointer_cast<ast::VariableDecl>(decl.declaration), decl.scope);
    }
  }

  mProcessingQueue.clear();
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


std::shared_ptr<program::Expression> ScriptCompiler::generateExpression(const std::shared_ptr<ast::Expression> & e)
{
  expr_.setScope(mCurrentScope);
  return expr_.generateExpression(e);
}

void ScriptCompiler::processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & class_decl)
{
  assert(class_decl != nullptr);

  ClassBuilder builder{ std::string{} };
  fill(builder, class_decl);

  Class cla = build(builder);
  cla.implementation()->script = script().weakref();
  currentScope().impl()->add_class(cla);

  readClassContent(cla, class_decl);
}

void ScriptCompiler::fill(ClassBuilder & builder, const std::shared_ptr<ast::ClassDecl> & decl)
{
  Class parent_class;
  if (decl->parent != nullptr)
  {
    NameLookup lookup = resolve(decl->parent);
    if (lookup.resultType() != NameLookup::TypeName || !lookup.typeResult().isObjectType())
      throw InvalidBaseClass{ dpos(decl->parent) };
    parent_class = engine()->getClass(lookup.typeResult());
    assert(!parent_class.isNull());
  }

  builder.name = decl->name->getName();
  builder.setParent(parent_class);
}

void ScriptCompiler::readClassContent(Class & c, const std::shared_ptr<ast::ClassDecl> & decl)
{
  Scope class_scope = Scope{ c, currentScope() };
  for (size_t i(0); i < decl->content.size(); ++i)
  {
    if (decl->content.at(i)->is<ast::Declaration>())
      processOrCollectDeclaration(std::static_pointer_cast<ast::Declaration>(decl->content.at(i)), class_scope);
    else if (decl->content.at(i)->is<ast::AccessSpecifier>())
    {
      log(diagnostic::warning() << "Access specifiers are ignored for type members");
      AccessSpecifier aspec = get_access_specifier(std::static_pointer_cast<ast::AccessSpecifier>(decl->content.at(i)));
      class_scope = Scope{ std::static_pointer_cast<ClassScope>(class_scope.impl())->withAccessibility(aspec) };
    }
  }
}


void ScriptCompiler::processEnumDeclaration(const std::shared_ptr<ast::EnumDeclaration> & decl)
{
  const Scope scp = currentScope();
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

void ScriptCompiler::processTypedef(const std::shared_ptr<ast::Typedef> & decl)
{
  const ast::Typedef & tdef = *decl;

  const Type t = resolve(tdef.qualified_type);
  const std::string & name = tdef.name->getName();

  currentScope().impl()->add_typedef(Typedef{ name, t });
}

void ScriptCompiler::processNamespaceDecl(const std::shared_ptr<ast::NamespaceDeclaration> & decl)
{
  const Scope scp = currentScope();
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

void ScriptCompiler::processImportDirective(const std::shared_ptr<ast::ImportDirective> & decl)
{
  if (decl->export_keyword.isValid())
  {
    log(diagnostic::info() << dpos(decl->export_keyword) << "'export' is ignored for now");
  }

  Scope imported = modules_.process(decl);
  mCurrentScope.merge(imported);
}

void ScriptCompiler::processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  const Scope scp = currentScope();
  FunctionBuilder builder = FunctionBuilder::Function(fundecl->name->getName(), Prototype{});
  function_processor_.fill(builder, fundecl, scp);
  Function function = build(builder);

  scp.impl()->add_function(function);

  if (function.isVirtual() && !fundecl->virtualKeyword.isValid())
    log(diagnostic::warning() << diagnostic::pos(fundecl->pos().line, fundecl->pos().col)
      << "Function overriding base virtual member declared without virtual or override specifier");

  schedule(function, fundecl, scp);
}

void ScriptCompiler::processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl)
{
  const Scope scp = currentScope();
  Class current_class = scp.asClass();

  FunctionBuilder b = FunctionBuilder::Constructor(current_class, Prototype{});
  function_processor_.fill(b, decl, scp);
  Function ctor = build(b);

  /// TODO : be careful not to add the constructor twice
  // for now this is okay since the FunctionBuilder never adds anything to the class.
  current_class.implementation()->registerConstructor(ctor);

  schedule(ctor, decl, scp);
}

void ScriptCompiler::processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl)
{
  const Scope scp = currentScope();
  const auto & dtor_decl = *decl;
  Class current_class = scp.asClass();

  FunctionBuilder b = FunctionBuilder::Destructor(current_class);
  function_processor_.fill(b, decl, scp);

  if (!current_class.parent().isNull())
  {
    Function parent_dtor = current_class.parent().destructor();
    if (!parent_dtor.isNull() && parent_dtor.isVirtual())
      b.setVirtual();
  }

  /// TODO : check if a destructor already exists
  Function dtor = build(b);
  current_class.implementation()->destructor = dtor;
  
  schedule(dtor, decl, scp);
}

void ScriptCompiler::processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::OperatorOverloadDecl & over_decl = *decl;

  /// TODO : check that we are at namespace level !

  FunctionBuilder b = FunctionBuilder::Operator(Operator::Null);
  function_processor_.fill(b, decl, scp);

  std::string suffix_name = over_decl.name->as<ast::LiteralOperatorName>().suffix_string();

  LiteralOperator function{ std::make_shared<LiteralOperatorImpl>(std::move(suffix_name), b.proto, engine()) };

  scp.impl()->add_literal_operator(function);

  schedule(function, decl, scp);
}

void ScriptCompiler::processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::OperatorOverloadDecl & over_decl = *decl;

  if (over_decl.name->is<ast::LiteralOperatorName>())
    return processLiteralOperatorDecl(decl);

  FunctionBuilder builder = FunctionBuilder::Operator(Operator::Null, Prototype{});
  function_processor_.fill(builder, decl, scp);
  
  const bool is_member = currentScope().type() == script::Scope::ClassScope;
  
  auto arity = builder.proto.argc() == 2 ? ast::OperatorName::BuiltInOpResol::BinaryOp : ast::OperatorName::UnaryOp;
  builder.operation = ast::OperatorName::getOperatorId(over_decl.name->name, arity);
  if (builder.operation == Operator::Null)
  {
    // operator++(int) and operator++() trick
    if ((over_decl.name->name == parser::Token::PlusPlus || over_decl.name->name == parser::Token::MinusMinus)
      && (builder.proto.argc() == 2 && builder.proto.argv(1) == Type::Int))
    {
      builder.proto.popArgument();
      builder.operation = over_decl.name->name == parser::Token::PlusPlus ? Operator::PostIncrementOperator : Operator::PostDecrementOperator;
    }
    else
      throw CouldNotResolveOperatorName{ dpos(over_decl) };
  }

  if (Operator::isBinary(builder.operation) && builder.proto.argc() != 2)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), std::to_string(2), std::to_string(builder.proto.argc()) };
  else if (Operator::isUnary(builder.operation) && builder.proto.argc() != 1)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), std::to_string(1), std::to_string(builder.proto.argc()) };

  
  if (Operator::onlyAsMember(builder.operation) && !is_member)
    throw OpOverloadMustBeDeclaredAsMember{ dpos(over_decl) };

  Operator function = build(builder).toOperator();

  scp.impl()->add_operator(function);
  schedule(function, decl, scp);
}

void ScriptCompiler::processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::CastDecl & cast_decl = *decl;

  const bool is_member = scp.type() == script::Scope::ClassScope;
  assert(is_member); /// TODO : is this necessary (should be enforced by the parser)

  FunctionBuilder builder{ Function::CastFunction };
  function_processor_.fill(builder, decl, scp);

  Cast cast = build(builder).toCast();

  scp.impl()->add_cast(cast);
  
  schedule(cast, decl, scp);
}

void ScriptCompiler::processTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl)
{
  if (decl->is_full_specialization())
  {
    if (decl->declaration->is<ast::FunctionDecl>())
      return processFunctionTemplateFullSpecialization(decl, std::static_pointer_cast<ast::FunctionDecl>(decl->declaration));
    else
      return processClassTemplateFullSpecialization(decl, std::static_pointer_cast<ast::ClassDecl>(decl->declaration));
  }
  else if (decl->is_partial_specialization())
  {
    throw NotImplementedError{ dpos(decl), "Template partial specialization not implemented yet" };
  }
  else
  {
    if (decl->declaration->is<ast::FunctionDecl>())
      return processFunctionTemplateDeclaration(decl, std::static_pointer_cast<ast::FunctionDecl>(decl->declaration));
    else
      return processClassTemplateDeclaration(decl, std::static_pointer_cast<ast::ClassDecl>(decl->declaration));
  }
}

std::vector<TemplateParameter> ScriptCompiler::processTemplateParameters(const std::shared_ptr<ast::TemplateDeclaration> & decl)
{
  std::vector<TemplateParameter> result;

  for (size_t i(0); i < decl->parameters.size(); ++i)
  {
    const auto & p = decl->parameters.at(i);

    if (p.kind == parser::Token::Typename)
    {
      result.push_back(TemplateParameter{ TemplateParameter::TypeParameter{}, decl->parameter_name(i) });
    }
    else if (p.kind == parser::Token::Bool)
    {
      result.push_back(TemplateParameter{ Type::Boolean, decl->parameter_name(i) });
    }
    else if (p.kind == parser::Token::Int)
    {
      result.push_back(TemplateParameter{ Type::Int, decl->parameter_name(i) });
    }
    else
      throw NotImplementedError{ dpos(decl), "Invalid template parameter" };

    result.back().setDefaultValue(p.default_value);
  }

  return result;
}

void ScriptCompiler::processClassTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl)
{
  Scope scp = currentScope();

  const std::string name = classdecl->name->getName();
  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  ClassTemplate ct = engine()->newClassTemplate(name, std::move(params), scp, nullptr);
  TemplateDefinition tdef = TemplateDefinition::make(decl);
  ct.impl()->definition = tdef;
  ct.impl()->script = script().weakref();
  scp.impl()->add_template(ct);
}

void ScriptCompiler::processFunctionTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  Scope scp = currentScope();

  const std::string name = fundecl->name->getName();
  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  FunctionTemplate ft = engine()->newFunctionTemplate(name, std::move(params), scp, nullptr, nullptr, nullptr);
  TemplateDefinition tdef = TemplateDefinition::make(decl);
  ft.impl()->definition = tdef;
  ft.impl()->script = script().weakref();
  scp.impl()->add_template(ft);
}

Namespace ScriptCompiler::findEnclosingNamespace(const Scope & scp) const
{
  if (scp.type() == Scope::NamespaceScope)
    return scp.asNamespace();
  else if (scp.isClass())
    return scp.asClass().enclosingNamespace();
  return findEnclosingNamespace(scp.parent());
}

ClassTemplate ScriptCompiler::findClassTemplate(const std::string & name, const std::vector<Template> & list)
{
  for (const auto & t : list)
  {
    if (t.isClassTemplate() && t.name() == name)
      return t.asClassTemplate();
  }

  return ClassTemplate{};
}

void ScriptCompiler::processClassTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl)
{
  Scope scp = currentScope();

  auto template_name = ast::Identifier::New(classdecl->name->name, classdecl->name->ast.lock());
  /// TODO : set a flag in the name resolver to prevent template instantiation and use the template name directly
  Namespace ns = findEnclosingNamespace(scp);
  ClassTemplate ct = findClassTemplate(classdecl->name->getName(), ns.templates());

  if (ct.isNull())
    throw CouldNotFindPrimaryClassTemplate{dpos(classdecl)};

  TemplateNameProcessor tnp; /// TODO : use a custom TNP
  auto template_full_name = std::static_pointer_cast<ast::TemplateIdentifier>(classdecl->name);
  std::vector<TemplateArgument> args = tnp.arguments(scp, template_full_name->arguments);

  ClassBuilder builder{ std::string{} };
  fill(builder, classdecl);

  Class result{ ClassTemplateInstance::make(builder, ct, args) };
  result.implementation()->script = script().weakref();

  readClassContent(result, classdecl);

  ct.impl()->instances[args] = result;
}

void ScriptCompiler::processFunctionTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  throw NotImplementedError{ dpos(decl), "Full specialization of function template not supported yet" };
}

void ScriptCompiler::reprocess(const IncompleteFunction & func)
{
  const auto & decl = std::static_pointer_cast<ast::FunctionDecl>(func.declaration);
  mCurrentScope = func.scope;

  auto impl = func.function.implementation();
  Prototype & proto = impl->prototype;
  if (proto.returnType().testFlag(Type::UnknownFlag))
    proto.setReturnType(resolve(decl->returnType));

  const int offset = proto.argc() > 0 && proto.argv(0).testFlag(Type::ThisFlag) ? 1 : 0;

  for (int i(0); i < proto.argc(); ++i)
  {
    if (proto.argv(i).testFlag(Type::UnknownFlag))
      proto.setParameter(i, resolve(decl->params.at(i-offset).type));

    const bool optional = decl->params.at(i - offset).defaultValue != nullptr;
    if(optional)
      proto.setParameter(i, proto.argv(i).withFlag(Type::OptionalFlag));
  }
}

void ScriptCompiler::schedule(const Function & f, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
{
  if (function_processor_.prototype_.type_.relax)
    mIncompleteFunctions.push_back(IncompleteFunction{ scp, fundecl, f });
  function_processor_.prototype_.type_.relax = false;

  if (f.isDeleted() || f.isPureVirtual())
    return;
  mCompilationTasks.push_back(CompileFunctionTask{ f, fundecl, scp });
}

Class ScriptCompiler::build(const ClassBuilder & builder)
{
  Class cla = CompilerComponent::build(builder);
  cla.implementation()->script = mCurrentScript.weakref();
  return cla;
}

Function ScriptCompiler::build(const FunctionBuilder & builder)
{
  Function f = CompilerComponent::build(builder);
  f.implementation()->script = mCurrentScript.weakref();
  return f;
}

Enum ScriptCompiler::build(const Enum &, const std::string & name)
{
  Enum e = CompilerComponent::build(Enum{}, name);
  e.implementation()->script = mCurrentScript.weakref();
  return e;
}

} // namespace compiler

} // namespace script

