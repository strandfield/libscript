// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/scriptcompiler.h"

#include "script/compiler/compilererrors.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functioncompiler.h"

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
#include "../scope_p.h"
#include "../script_p.h"

namespace script
{

namespace compiler
{

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

ScriptCompiler::ScriptCompiler(Compiler *c, CompileSession *s)
  : CompilerComponent(c, s)
  , variable_(c->engine())
{
  name_resolver.compiler = this;
  type_resolver.name_resolver() = name_resolver;
  lenient_resolver.name_resolver() = name_resolver;
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
      break;
    case ast::NodeType::CastDeclaration:
      throw NotImplementedError{ "ScriptCompiler::registerRootFunction() : cast declaration are not allowed at this scope" };
    default:
      fakedecl->body->statements.push_back(s);
      break;
    }
  }

  schedule(CompileFunctionTask{ Function{scriptfunc}, fakedecl, mCurrentScope });

  return scriptfunc;
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
    processUsingDirective(std::static_pointer_cast<ast::UsingDirective>(declaration));
    break;
  case ast::NodeType::UsingDeclaration:
    processUsingDeclaration(std::static_pointer_cast<ast::UsingDeclaration>(declaration));
    break;
  case ast::NodeType::NamespaceAliasDef:
    processNamespaceAlias(std::static_pointer_cast<ast::NamespaceAliasDefinition>(declaration));
    break;
  case ast::NodeType::TypeAliasDecl:
    processTypeAlias(std::static_pointer_cast<ast::TypeAliasDeclaration>(declaration));
    break;
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

static inline AccessSpecifier get_access_specifier(const std::shared_ptr<ast::AccessSpecifier> & as)
{
  if (as->visibility == parser::Token::Private)
    return AccessSpecifier::Private;
  else if (as->visibility == parser::Token::Protected)
    return AccessSpecifier::Protected;
  return AccessSpecifier::Public;
}

void ScriptCompiler::processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & class_decl)
{
  assert(class_decl != nullptr);

  const Scope scp = currentScope();
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

void ScriptCompiler::processUsingDirective(const std::shared_ptr<ast::UsingDirective> & decl)
{
  throw NotImplementedError{ dpos(decl), "Using directive not implemented yet" };
}

void ScriptCompiler::processUsingDeclaration(const std::shared_ptr<ast::UsingDeclaration> & decl)
{
  NameLookup lookup = resolve(decl->used_name);
  /// TODO : throw exception if nothing found
  mCurrentScope.inject(lookup.impl().get());
}

void ScriptCompiler::processNamespaceAlias(const std::shared_ptr<ast::NamespaceAliasDefinition> & decl)
{
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

void ScriptCompiler::processTypeAlias(const std::shared_ptr<ast::TypeAliasDeclaration> & decl)
{
  /// TODO : check that alias_name is a simple identifier or enforce it in the parser
  const std::string & name = decl->alias_name->getName();

  NameLookup lookup = resolve(decl->aliased_type);
  if (lookup.typeResult().isNull())
    throw InvalidTypeName{ dpos(decl), dstr(decl->aliased_type) };

  mCurrentScope.inject(name, lookup.typeResult());
}

void ScriptCompiler::processImportDirective(const std::shared_ptr<ast::ImportDirective> & decl)
{
  if (decl->export_keyword.isValid())
  {
    log(diagnostic::info() << dpos(decl->export_keyword) << "'export' is ignored for now");
  }

  Module m = engine()->getModule(decl->at(0));
  if (m.isNull())
  {
    load_script_module(decl);
    return; 
  }

  for (size_t i(1); i < decl->size(); ++i)
  {
    Module child = m.getSubModule(decl->at(i));
    if(child.isNull())
      throw UnknownSubModuleName{ dpos(decl), decl->at(i), m.name() };

    m = child;
  }

  m.load();

  mCurrentScope.merge(m.scope());
}

void ScriptCompiler::handleAccessSpecifier(FunctionBuilder &builder, const Scope & scp)
{
  switch (scp.accessibility())
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

void ScriptCompiler::processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  const Scope scp = currentScope();
  FunctionBuilder builder = FunctionBuilder::Function(fundecl->name->getName(), functionPrototype(fundecl));
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

  schedule_for_reprocessing(fundecl, function);
}

void ScriptCompiler::processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl)
{
  const Scope scp = currentScope();
  const auto & ctor_decl = *decl;
  Class current_class = scp.asClass();

  Prototype proto = functionPrototype(decl);

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

  schedule_for_reprocessing(decl, ctor);
}

void ScriptCompiler::processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl)
{
  const Scope scp = currentScope();
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

  schedule_for_reprocessing(decl, dtor);
}

void ScriptCompiler::processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::OperatorOverloadDecl & over_decl = *decl;

  /// TODO : check that we are at namespace level !

  Prototype proto = functionPrototype(decl);

  std::string suffix_name = over_decl.name->as<ast::LiteralOperatorName>().suffix_string();

  LiteralOperator function{ std::make_shared<LiteralOperatorImpl>(std::move(suffix_name), proto, engine()) };

  scp.impl()->add_literal_operator(function);

  if (!function.isDeleted())
    schedule(CompileFunctionTask{ function, decl, scp });

  schedule_for_reprocessing(decl, function);
}

void ScriptCompiler::processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::OperatorOverloadDecl & over_decl = *decl;

  if (over_decl.name->is<ast::LiteralOperatorName>())
    return processLiteralOperatorDecl(decl);

  Prototype proto = functionPrototype(decl);
  
  const bool is_member = currentScope().type() == script::Scope::ClassScope;
  
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
  else if (over_decl.defaultKeyword.isValid())
    builder.setDefaulted();

  handleAccessSpecifier(builder, scp);

  Operator function = build(builder).toOperator();

  scp.impl()->add_operator(function);
  
  if(!function.isDeleted())
    schedule(CompileFunctionTask{ function, decl, scp });

  schedule_for_reprocessing(decl, function);
}

void ScriptCompiler::processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::CastDecl & cast_decl = *decl;

  Prototype proto = functionPrototype(decl);

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

  schedule_for_reprocessing(decl, cast);
}

Prototype ScriptCompiler::functionPrototype(const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  lenient_resolver.relax = false;

  const Scope scp = currentScope();

  Prototype result;

  if (fundecl->is<ast::ConstructorDecl>())
    result.setReturnType(Type{ scp.asClass().id(), Type::ReferenceFlag | Type::ConstFlag });
  else if (fundecl->is<ast::DestructorDecl>())
    result.setReturnType(Type::Void);
  else
    result.setReturnType(lenient_resolver.resolve(fundecl->returnType));

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
    Type argtype = lenient_resolver.resolve(fundecl->params.at(i).type);
    if (fundecl->params.at(i).defaultValue != nullptr)
      argtype.setFlag(Type::OptionalFlag), mustbe_defaulted = true;
    else if (mustbe_defaulted)
      throw InvalidUseOfDefaultArgument{ dpos(fundecl->params.at(i).name) };
    result.addArgument(argtype);
  }

  return result;
}

void ScriptCompiler::schedule_for_reprocessing(const std::shared_ptr<ast::FunctionDecl> & decl, const Function & f)
{
  if (lenient_resolver.relax)
    mIncompleteFunctions.push_back(IncompleteFunction{ currentScope(), decl, f });

  lenient_resolver.relax = false;
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

    bool optional = decl->params.at(i - offset).defaultValue != nullptr;
    proto.setParameter(i, proto.argv(i).withFlag(Type::OptionalFlag));
  }
}

void ScriptCompiler::load_script_module(const std::shared_ptr<ast::ImportDirective> & decl)
{
  auto path = engine()->searchDirectory();

  for (size_t i(0); i < decl->size(); ++i)
    path /= decl->at(i);

  if (support::filesystem::is_directory(path))
  {
    load_script_module_recursively(path);
  }
  else
  {
    path += engine()->scriptExtension();

    if (!support::filesystem::exists(path))
      throw UnknownModuleName{ dpos(decl), decl->full_name() };

    load_script_module(path);
  }
}

void ScriptCompiler::load_script_module(const support::filesystem::path & p)
{
  if (p.extension() != engine()->scriptExtension())
    return;

  Script s;
  if (is_loaded(p, s))
  {
    mCurrentScope.merge(Scope{s.rootNamespace()});
    return;
  }

  s = engine()->newScript(SourceFile{ p.string() });

  parser::Parser parser{ s.source() };
  auto ast = parser.parse(s.source());

  assert(ast != nullptr);

  if (ast->hasErrors())
  {
    log(diagnostic::info() << "While loading script module:");
    for (const auto & m : ast->messages())
      log(m);
    
    /// TODO : destroy scripts
    return;
  }

  mTasks.push_back(CompileScriptTask{s, ast});
  processOrCollectScriptDeclarations(mTasks.back());

  mCurrentScope.merge(Scope{ s.rootNamespace() });
}

void ScriptCompiler::load_script_module_recursively(const support::filesystem::path & dir)
{
  for (auto& p : support::filesystem::directory_iterator(dir))
  {
    if (support::filesystem::is_directory(p))
      load_script_module_recursively(p);
    else
      load_script_module(p);
  }
}

bool ScriptCompiler::is_loaded(const support::filesystem::path & p, Script & result)
{
  for (const auto & s : engine()->scripts())
  {
    if (s.path() == p)
    {
      result = s;
      return true;
    }
  }

  return false;
}

void ScriptCompiler::schedule(const CompileFunctionTask & task)
{
  mCompilationTasks.push_back(task);
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

