// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/scriptcompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/templatedefinition.h"
#include "script/compiler/templatespecialization.h"

#include "script/ast/ast_p.h"
#include "script/ast/node.h"

#include "script/parser/parser.h"

#include "script/program/expression.h"

#include "script/castbuilder.h"
#include "script/classbuilder.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/classtemplatespecializationbuilder.h"
#include "script/constructorbuilder.h"
#include "script/destructorbuilder.h"
#include "script/enumbuilder.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/function_p.h"
#include "script/functiontype.h"
#include "script/literals.h"
#include "script/literaloperatorbuilder.h"
#include "script/operatorbuilder.h"
#include "script/private/script_p.h"
#include "script/private/template_p.h"
#include "script/symbol.h"
#include "script/templatebuilder.h"
#include "script/templatenameprocessor.h"

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
{

}

ScriptCompiler::StateGuard::~StateGuard()
{
  compiler->mCurrentScope = scope;
  compiler->mCurrentScript = script;
}

ScriptCompiler::ScriptCompiler(Compiler *c)
  : Component(c)
  , variable_(c)
  , modules_(c->engine())
  , default_arguments_(c)
  , mReprocessingIncompleteFunctions(false)
{

  scope_statements_.scope_ = &mCurrentScope;
}

ScriptCompiler::~ScriptCompiler()
{

}

void ScriptCompiler::add(const Script & task)
{
  parser::Parser parser{ task.source() };
  auto ast = parser.parse(task.source());

  if (ast->hasErrors)
  {
    for (const auto & m : ast->messages)
      log(m);
    return; /// TODO: should we throw instead ?
  }

  task.impl()->ast = ast;
  ast->script = task.impl();

  processOrCollectScriptDeclarations(task);
}

Class ScriptCompiler::instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  TemplateSpecializationSelector selector;
  auto selected_specialization = selector.select(ct, args);
  if (!selected_specialization.first.isNull())
  {
    ClassTemplateSpecializationBuilder builder = ClassTemplate{ ct }.Specialization(std::vector<TemplateArgument>{args});

    auto class_decl = selected_specialization.first.impl()->definition.get_class_decl();
    builder.name = readClassName(class_decl);
    builder.base = readClassBase(class_decl);

    Class result = builder.get();

    StateGuard guard{ this };
    mCurrentScope = selected_specialization.first.argumentScope(selected_specialization.second);

    readClassContent(result, class_decl);

    return result;
  }
  else
  {
    ClassTemplateSpecializationBuilder builder = ClassTemplate{ ct }.Specialization(std::vector<TemplateArgument>{args});

    auto class_decl = static_cast<ScriptClassTemplateBackend*>(ct.backend())->definition.get_class_decl();
    builder.name = readClassName(class_decl);
    builder.base = readClassBase(class_decl);

    Class result = builder.get();

    StateGuard guard{ this };
    mCurrentScope = ct.argumentScope(args);

    readClassContent(result, class_decl);

    return result;
  }
}

bool ScriptCompiler::done() const
{
  return mIncompleteFunctionDeclarations.empty()
    && mProcessingQueue.empty();
}

void ScriptCompiler::processNext()
{
  if (!mIncompleteFunctionDeclarations.empty())
  {
    mReprocessingIncompleteFunctions = true;

    do
    {
      auto task = mIncompleteFunctionDeclarations.front();
      mIncompleteFunctionDeclarations.pop();

      reprocess(task);
    } while (!mIncompleteFunctionDeclarations.empty());

    mReprocessingIncompleteFunctions = false;

    return;
  }

  if (!mProcessingQueue.empty())
  {
    auto task = mProcessingQueue.front();
    mProcessingQueue.pop();

    ScopeGuard guard{ mCurrentScope };
    mCurrentScope = task.scope;

    if (task.declaration->is<ast::FriendDeclaration>())
    {
      processFriendDecl(std::static_pointer_cast<ast::FriendDeclaration>(task.declaration));
    }
    else
    {
      variable_.process(std::static_pointer_cast<ast::VariableDecl>(task.declaration), task.scope);
    }

    return;
  }
}

Type ScriptCompiler::resolve(const ast::QualifiedType & qt)
{
  return type_resolver.resolve(qt, mCurrentScope);
}

NameLookup ScriptCompiler::resolve(const std::shared_ptr<ast::Identifier> & id)
{
  return NameLookup::resolve(id, mCurrentScope);
}

Function ScriptCompiler::registerRootFunction()
{
  auto scriptfunc = std::make_shared<ScriptFunctionImpl>(engine());
  scriptfunc->enclosing_symbol = mCurrentScript.impl();

  auto fakedecl = ast::FunctionDecl::New(std::shared_ptr<ast::AST>());
  fakedecl->body = ast::CompoundStatement::New(parser::Token{ parser::Token::LeftBrace, 0, 0, 0, 0 }, parser::Token{ parser::Token::RightBrace, 0, 0, 0, 0 });
  const auto & stmts = currentAst()->root->as<ast::ScriptRootNode>().statements;
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
      throw NotImplemented{ "ScriptCompiler::registerRootFunction() : cast declaration are not allowed at this scope" };
    default:
      fakedecl->body->statements.push_back(s);
      break;
    }
  }

  Function ret{ scriptfunc };

  schedule(ret, fakedecl, mCurrentScope);

  return ret;
}

void ScriptCompiler::processOrCollectScriptDeclarations(const Script & task)
{
  StateGuard guard{ this };

  mCurrentScript = task;

  mCurrentScope = Scope{ mCurrentScript };
  mCurrentScope.merge(engine()->rootNamespace());

  Function program = registerRootFunction();
  mCurrentScript.impl()->program = program;
  processOrCollectScriptDeclarations();
}

bool ScriptCompiler::processOrCollectScriptDeclarations()
{
  for (const auto & decl : currentAst()->root->as<ast::ScriptRootNode>().declarations)
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
  case ast::NodeType::ConstructorDeclaration:
  case ast::NodeType::DestructorDeclaration:
  case ast::NodeType::OperatorOverloadDeclaration:
  case ast::NodeType::CastDeclaration:
    return processFunctionDeclaration(std::static_pointer_cast<ast::FunctionDecl>(declaration));
  case ast::NodeType::VariableDeclaration:
  case ast::NodeType::ClassFriendDecl:
    return collectDeclaration(declaration);
  case ast::NodeType::TemplateDecl:
    return processTemplateDeclaration(std::static_pointer_cast<ast::TemplateDeclaration>(declaration));
  default:
    throw NotImplemented{ "ScriptCompiler::processOrCollectDeclaration() : declaration not implemented" };
  }
}

void ScriptCompiler::collectDeclaration(const std::shared_ptr<ast::Declaration> & decl)
{
  mProcessingQueue.push(ScopedDeclaration{ currentScope(), decl });
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

  currentScope().asClass().addFriend(engine()->typeSystem()->getClass(lookup.typeResult()));
}

void ScriptCompiler::processPendingDeclarations()
{
  /*for (size_t i(0); i < mProcessingQueue.size(); ++i)
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

  mProcessingQueue.clear();*/
  
  while (!mProcessingQueue.empty())
  {
    auto task = mProcessingQueue.front();
    mProcessingQueue.pop();


    ScopeGuard guard{ mCurrentScope };
    mCurrentScope = task.scope;

    if (task.declaration->is<ast::FriendDeclaration>())
    {
      processFriendDecl(std::static_pointer_cast<ast::FriendDeclaration>(task.declaration));
    }
    else
    {
      variable_.process(std::static_pointer_cast<ast::VariableDecl>(task.declaration), task.scope);
    }


  }
}


void ScriptCompiler::processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & class_decl)
{
  assert(class_decl != nullptr);

  ClassBuilder builder = currentScope().symbol().newClass("");
  fill(builder, class_decl);

  Class cla = builder.get();
  mCurrentScope.invalidateCache(Scope::InvalidateClassCache);

  readClassContent(cla, class_decl);
}

std::string ScriptCompiler::readClassName(const std::shared_ptr<ast::ClassDecl> & decl)
{
  if (decl->name->is<ast::SimpleIdentifier>())
    return decl->name->as<ast::SimpleIdentifier>().getName();
  return decl->name->as<ast::TemplateIdentifier>().getName();
}

Type ScriptCompiler::readClassBase(const std::shared_ptr<ast::ClassDecl> & decl)
{
  if (decl->parent == nullptr)
    return Type{};

  NameLookup lookup = resolve(decl->parent);
  if (lookup.resultType() != NameLookup::TypeName || !lookup.typeResult().isObjectType())
    throw InvalidBaseClass{ dpos(decl->parent) };
  return lookup.typeResult();
}

void ScriptCompiler::fill(ClassBuilder & builder, const std::shared_ptr<ast::ClassDecl> & decl)
{
  builder.name = readClassName(decl);
  builder.setBase(readClassBase(decl));
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

  Symbol symbol = scp.symbol();

  Enum e = symbol.newEnum(enum_decl.name->getName()).get();

  mCurrentScope.invalidateCache(Scope::InvalidateEnumCache);

  for (size_t i(0); i < enum_decl.values.size(); ++i)
  {
    if (enum_decl.values.at(i).value != nullptr)
      throw NotImplemented{ "Enum value with initialization are not supported yet" };
    else
      e.addValue(enum_decl.values.at(i).name->getName());
  }
}

void ScriptCompiler::processTypedef(const std::shared_ptr<ast::Typedef> & decl)
{
  const ast::Typedef & tdef = *decl;

  const Type t = resolve(tdef.qualified_type);
  const std::string & name = tdef.name->getName();

  Symbol s = currentScope().symbol();
  s.newTypedef(t, name).create();

  mCurrentScope.invalidateCache(Scope::InvalidateTypedefCache);
}

void ScriptCompiler::processNamespaceDecl(const std::shared_ptr<ast::NamespaceDeclaration> & decl)
{
  const Scope scp = currentScope();
  const ast::NamespaceDeclaration & ndecl = *decl;

  if (!scp.isNamespace())
    throw NamespaceDeclarationCannotAppearAtThisLevel{ dpos(decl) };

  Namespace parent_ns = scp.asNamespace();
  const std::string name = ndecl.namespace_name->getName();

  Namespace ns = parent_ns.getNamespace(name);

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
  Scope imported = modules_.process(decl);

  if (decl->export_keyword.isValid())
  {
    Script s = script();
    if (s.impl()->exports.isNull())
      s.impl()->exports = imported;
    else
      s.impl()->exports.merge(imported);
  }

  mCurrentScope.merge(imported);
}

void ScriptCompiler::processFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & declaration)
{
  try
  {
    switch (declaration->type())
    {
    case ast::NodeType::FunctionDeclaration:
      return processBasicFunctionDeclaration(std::static_pointer_cast<ast::FunctionDecl>(declaration));
    case ast::NodeType::ConstructorDeclaration:
      return processConstructorDeclaration(std::static_pointer_cast<ast::ConstructorDecl>(declaration));
    case ast::NodeType::DestructorDeclaration:
      return processDestructorDeclaration(std::static_pointer_cast<ast::DestructorDecl>(declaration));
    case ast::NodeType::OperatorOverloadDeclaration:
      return processOperatorOverloadingDeclaration(std::static_pointer_cast<ast::OperatorOverloadDecl>(declaration));
    case ast::NodeType::CastDeclaration:
      return processCastOperatorDeclaration(std::static_pointer_cast<ast::CastDecl>(declaration));
    default:
      throw std::runtime_error{ "Bad call to ScriptCompiler::processFunctionDeclaration()" };
    }
  }
  catch (compiler::InvalidTypeName &)
  {
    if (mReprocessingIncompleteFunctions)
      throw;

    log(diagnostic::warning() << dpos(declaration) << "Type name could not be resolved, function will be reprocessed later");
    mIncompleteFunctionDeclarations.push(ScopedDeclaration{ currentScope(), declaration });
  }
}

void ScriptCompiler::processBasicFunctionDeclaration(const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  Scope scp = currentScope();
  FunctionBuilder builder = scp.symbol().newFunction(fundecl->name->as<ast::SimpleIdentifier>().getName());
  function_processor_.generic_fill(builder, fundecl, scp);
  default_arguments_.generic_process(fundecl->params, builder, scp);
  Function function = builder.get();

  scp.invalidateCache(Scope::InvalidateFunctionCache);

  if (function.isVirtual() && !fundecl->virtualKeyword.isValid())
    log(diagnostic::warning() << diagnostic::pos(fundecl->pos().line, fundecl->pos().col)
      << "Function overriding base virtual member declared without virtual or override specifier");

  schedule(function, fundecl, scp);
}

void ScriptCompiler::processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl)
{
  const Scope scp = currentScope();
  Class current_class = scp.asClass();

  auto b = current_class.newConstructor();
  function_processor_.generic_fill(b, decl, scp);
  default_arguments_.generic_process(decl->params, b, scp);
  Function ctor = b.get();

  schedule(ctor, decl, scp);
}

void ScriptCompiler::processDestructorDeclaration(const std::shared_ptr<ast::DestructorDecl> & decl)
{
  const Scope scp = currentScope();
  Class current_class = scp.asClass();

  auto b = current_class.newDestructor();
  function_processor_.generic_fill(b, decl, scp);

  if (!current_class.parent().isNull())
  {
    Function parent_dtor = current_class.parent().destructor();
    if (!parent_dtor.isNull() && parent_dtor.isVirtual())
      b.setVirtual();
  }

  /// TODO : check if a destructor already exists
  Function dtor = b.get();
  
  schedule(dtor, decl, scp);
}

void ScriptCompiler::processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  Scope scp = currentScope().escapeTemplate();

  if (!scp.isNamespace())
    throw LiteralOperatorNotInNamespace{ dpos(decl) };

  std::string suffix_name = decl->name->as<ast::LiteralOperatorName>().suffix_string();

  auto b = scp.asNamespace().newUserDefinedLiteral(suffix_name);
  function_processor_.generic_fill(b, decl, currentScope());

  /// TODO: check that the user does not declare any default arguments

  Function function = b.get();

  scp.invalidateCache(Scope::InvalidateLiteralOperatorCache);

  schedule(function, decl, scp);
}

void ScriptCompiler::processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  Scope scp = currentScope();
  const ast::OperatorOverloadDecl & over_decl = *decl;

  if (over_decl.name->is<ast::LiteralOperatorName>())
    return processLiteralOperatorDecl(decl);

  const parser::Token op_symbol = over_decl.name->as<ast::OperatorName>().symbol;

  size_t arity = (scp.isClass() ? 1 : 0) + decl->params.size();
  auto search_option = arity == 1 ? ast::OperatorName::BuiltInOpResol::UnaryOp :
    (arity == 2 ? ast::OperatorName::BuiltInOpResol::BinaryOp : ast::OperatorName::BuiltInOpResol::All);
  OperatorName opname = ast::OperatorName::getOperatorId(op_symbol, search_option);

  if (opname == Operator::Null)
  {
    // operator++(int) and operator++() trick
    if ((op_symbol == parser::Token::PlusPlus || op_symbol == parser::Token::MinusMinus)
      && (arity == 2 && decl->params.at(0).type.type->as<ast::SimpleIdentifier>().name == parser::Token::Int))
    {
      opname = op_symbol == parser::Token::PlusPlus ? PostIncrementOperator : PostDecrementOperator;
    }
    else
      throw CouldNotResolveOperatorName{ dpos(over_decl) };
  }

  if (opname == OperatorName::FunctionCallOperator)
    return processFunctionCallOperatorDecl(decl);

  auto builder = scp.symbol().newOperator(opname);
  function_processor_.generic_fill(builder, decl, scp);
  
  const bool is_member = currentScope().isClass();

  if (Operator::isBinary(builder.operation) && arity != 2)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), 2, int(arity) };
  else if (Operator::isUnary(builder.operation) && builder.proto_.count() != 1)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), 1, int(arity) };

  if (Operator::onlyAsMember(builder.operation) && !is_member)
    throw OpOverloadMustBeDeclaredAsMember{ dpos(over_decl), builder.operation };

  /// TODO: check that the user does not declare any default arguments

  Function function = builder.get();

  scp.invalidateCache(Scope::InvalidateOperatorCache);

  schedule(function, decl, scp);
}

void ScriptCompiler::processFunctionCallOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  Scope scp = currentScope();

  auto builder = scp.symbol().toClass().newFunctionCallOperator();
  function_processor_.generic_fill(builder, decl, scp);
  default_arguments_.generic_process(decl->params, builder, scp);

  Function function = builder.get();
  scp.invalidateCache(Scope::InvalidateOperatorCache);
  schedule(function, decl, scp);
}

void ScriptCompiler::processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl)
{
  const Scope scp = currentScope();

  const bool is_member = scp.isClass();
  assert(is_member); /// TODO : is this necessary (should be enforced by the parser)

  auto builder = scp.symbol().toClass().newConversion(Type::Null);
  function_processor_.generic_fill(builder, decl, scp);
  /// TODO: check that the user does not declare any default arguments
  Function cast = builder.get();
  
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
    return processClassTemplatePartialSpecialization(decl, std::static_pointer_cast<ast::ClassDecl>(decl->declaration));
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
    {
      throw NotImplemented{ "Invalid template parameter" };
    }

    result.back().setDefaultValue(p.default_value);
  }

  return result;
}

void ScriptCompiler::processClassTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl)
{
  Scope scp = currentScope();

  std::string name = classdecl->name->as<ast::SimpleIdentifier>().getName();
  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  ClassTemplate ct = scp.symbol().newClassTemplate(std::move(name))
    .setParams(std::move(params))
    .setScope(scp)
    .withBackend<ScriptClassTemplateBackend>()
    .get();

  scp.invalidateCache(Scope::InvalidateTemplateCache);

  static_cast<ScriptClassTemplateBackend*>(ct.backend())->definition = TemplateDefinition::make(script(), decl);
}

void ScriptCompiler::processFunctionTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  Scope scp = currentScope();

  std::string name = fundecl->name->as<ast::SimpleIdentifier>().getName();
  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  FunctionTemplate ft = scp.symbol().newFunctionTemplate(std::move(name))
    .setParams(std::move(params))
    .setScope(scp)
    .withBackend<ScriptFunctionTemplateBackend>()
    .get();

  scp.invalidateCache(Scope::InvalidateTemplateCache);

  static_cast<ScriptFunctionTemplateBackend*>(ft.backend())->definition = TemplateDefinition::make(script(), decl);
}

Namespace ScriptCompiler::findEnclosingNamespace(const Scope & scp) const
{
  if (scp.isNamespace())
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

  Namespace ns = findEnclosingNamespace(scp);
  ClassTemplate ct = findClassTemplate(classdecl->name->as<ast::TemplateIdentifier>().getName(), ns.templates());

  if (ct.isNull())
    throw CouldNotFindPrimaryClassTemplate{ dpos(classdecl) };

  auto template_full_name = std::static_pointer_cast<ast::TemplateIdentifier>(classdecl->name);
  std::vector<TemplateArgument> args = TemplateNameProcessor::arguments(scp, template_full_name->arguments);

  ClassTemplateSpecializationBuilder builder = ct.Specialization(std::move(args));
  builder.name = readClassName(classdecl);
  builder.base = readClassBase(classdecl);

  Class result = builder.get(); /// TODO: record this generated template instance

  readClassContent(result, classdecl);

  ct.impl()->instances[args] = result;
}

void ScriptCompiler::processClassTemplatePartialSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl)
{
  assert(decl->is_partial_specialization());

  Scope scp = currentScope();

  Namespace ns = findEnclosingNamespace(scp);
  ClassTemplate ct = findClassTemplate(classdecl->name->as<ast::TemplateIdentifier>().getName(), ns.templates());

  if (ct.isNull())
    throw CouldNotFindPrimaryClassTemplate{ dpos(classdecl) };

  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  auto ps = std::make_shared<PartialTemplateSpecializationImpl>(ct, std::move(params), scp, engine(), scp.symbol().impl());
  ps->definition = TemplateDefinition::make(script(), decl);

  auto* back = dynamic_cast<ScriptClassTemplateBackend*>(ct.backend());

  back->specializations.push_back(PartialTemplateSpecialization(ps));
}

void ScriptCompiler::processFunctionTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  assert(decl->is_full_specialization());

  const Scope scp = currentScope();

  Namespace ns = findEnclosingNamespace(scp);

  const std::vector<Template> & tmplts = ns.templates();

  if (tmplts.empty())
    throw CouldNotFindPrimaryFunctionTemplate{ dpos(fundecl) };

  std::vector<TemplateArgument> args;
  if (fundecl->name->is<ast::TemplateIdentifier>())
  {
    auto template_full_name = std::static_pointer_cast<ast::TemplateIdentifier>(fundecl->name);
    args = TemplateNameProcessor::arguments(scp, template_full_name->arguments);
  }

  FunctionBuilder builder{ scp.symbol(), std::string{} };
  function_processor_.generic_fill(builder, fundecl, scp);
  /// TODO : the previous statement may throw an exception if some type name cannot be resolved, 
  // to avoid this error, we should process all specializations at the end !

  TemplateOverloadSelector selector;
  auto selection = selector.select(tmplts, args, builder.proto_);

  if(selection.first.isNull())
    throw CouldNotFindPrimaryFunctionTemplate{ dpos(fundecl) };

  /// TODO : merge this duplicate of FunctionTemplateProcessor
  /// TODO: handle default arguments
  auto impl = std::make_shared<FunctionTemplateInstance>(selection.first, selection.second, builder.name_, builder.proto_, engine(), builder.flags);
  impl->implementation = builder.body;
  impl->data = builder.data;
  impl->enclosing_symbol = scp.symbol().impl();
  Function result = Function{ impl };

  schedule(result, fundecl, scp);

  /// TODO : should this be done now or after full compilation ?
  selection.first.impl()->instances[selection.second] = result;
}

void ScriptCompiler::reprocess(ScopedDeclaration & func)
{
  ScopeGuard guard{ mCurrentScope };
  mCurrentScope = func.scope;

  processOrCollectDeclaration(func.declaration);
}

void ScriptCompiler::schedule(Function & f, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
{
  if (f.isDeleted() || f.isPureVirtual())
    return;
  mCompilationTasks.push(CompileFunctionTask{ f, fundecl, scp });
}

const std::shared_ptr<ast::AST> & ScriptCompiler::currentAst() const
{
  return mCurrentScript.impl()->ast;
}

} // namespace compiler

} // namespace script

