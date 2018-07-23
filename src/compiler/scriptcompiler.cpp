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

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/parser/parser.h"

#include "script/program/expression.h"

#include "script/classbuilder.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/classtemplatespecializationbuilder.h"
#include "script/enumbuilder.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/function_p.h"
#include "script/functiontype.h"
#include "script/literals.h"
#include "script/private/script_p.h"
#include "script/private/template_p.h"
#include "script/symbol.h"
#include "script/templatebuilder.h"

namespace script
{

namespace compiler
{

class ScriptCompilerTemplateNameProcessor : public TemplateNameProcessor
{
public:
  ScriptCompiler* compiler_;
public:
  ScriptCompilerTemplateNameProcessor(ScriptCompiler *c)
    : compiler_(c) { }

  ~ScriptCompilerTemplateNameProcessor() = default;

  Class instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args) override;
};

Class ScriptCompilerTemplateNameProcessor::instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  if (ct.is_native())
  {
    auto instantiate = ct.native_callback();
    ClassTemplateInstanceBuilder builder{ ct, std::vector<TemplateArgument>{ args} };
    Class ret = instantiate(builder);
    ct.impl()->instances[args] = ret;
    compiler_->session()->generated.classes.push_back(ret);
    return ret;
  }
  else
  {
    Class ret = compiler_->instantiate(ct, args, ScriptCompilerComponentKey{});
    ct.impl()->instances[args] = ret;
    return ret;
  }
}


NameLookup ScriptCompilerNameResolver::resolve(const std::shared_ptr<ast::Identifier> & name)
{
  return NameLookup::resolve(name, compiler->currentScope(), *tnp);
}

NameLookup ScriptCompilerNameResolver::resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scp)
{
  return NameLookup::resolve(name, scp, *tnp);
}


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

Engine* ScriptCompilerModuleLoader::engine() const
{
  return compiler_->engine();
}

Script ScriptCompilerModuleLoader::load(const SourceFile &src)
{
  Script s = engine()->newScript(src);

  compiler_->load(s, ScriptCompilerComponentKey{});

  return s;
}

ScriptCompiler::ScriptCompiler(Compiler *c)
  : CompilerComponent(c)
  , variable_(c->engine())
{
  tnp_ = std::make_unique<ScriptCompilerTemplateNameProcessor>(this);

  name_resolver.compiler = this;
  name_resolver.tnp = tnp_.get();

  type_resolver.name_resolver() = name_resolver;

  function_processor_.prototype_.type_.name_resolver() = name_resolver;

  scope_statements_.scope_ = &mCurrentScope;

  modules_.loader_.compiler_ = this;
}

ScriptCompiler::~ScriptCompiler()
{

}

void ScriptCompiler::compile(const Script & task)
{
  add(task);

  resolveIncompleteTypes();
  processPendingDeclarations();
  compileFunctions();
  //initializeStaticVariables();
  variable_.initializeVariables();

  for (auto s : session()->generated.scripts)
    s.run();
}

void ScriptCompiler::add(const Script & task)
{
  parser::Parser parser{ task.source() };
  auto ast = parser.parse(task.source());

  if (ast->hasErrors())
  {
    session()->messages = ast->steal_messages();
    session()->error = true;
    return;
  }

  task.impl()->ast = ast;

  processOrCollectScriptDeclarations(task);
}

Class ScriptCompiler::instantiate2(const ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  return instantiate(ct, args, ScriptCompilerComponentKey{});
}

bool ScriptCompiler::done() const
{
  return mIncompleteFunctions.empty()
    && mProcessingQueue.empty()
    && mCompilationTasks.empty()
    && variable_.empty();
}

void ScriptCompiler::processNext()
{
  if (!mIncompleteFunctions.empty())
  {
    while (!mIncompleteFunctions.empty())
    {
      auto task = mIncompleteFunctions.front();
      mIncompleteFunctions.pop();

      reprocess(task);
    }

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

  if (!mCompilationTasks.empty())
  {
    auto task = mCompilationTasks.front();
    mCompilationTasks.pop();

    FunctionCompiler fcomp{ compiler() };
    fcomp.compile(task);
    return;
  }


  if (!variable_.empty())
  {
    variable_.initializeVariables();
  }
}

Class ScriptCompiler::instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  Class result = instantiate(ct, args, ScriptCompilerComponentKey{});

  // This is a standalone job 
  resolveIncompleteTypes();
  processPendingDeclarations();
  compileFunctions();
  variable_.initializeVariables();

  for (auto s : session()->generated.scripts)
    s.run();

  return result;
}

void ScriptCompiler::load(const Script & s, ScriptCompilerComponentKey)
{
  parser::Parser parser{ s.source() };
  auto ast = parser.parse(s.source());

  s.impl()->ast = ast;

  if (ast->hasErrors())
  {
    log(diagnostic::info() << "While loading script module:");
    for (const auto & m : ast->messages())
      log(m);

    /// TODO : destroy scripts
    return;
  }

  session()->generated.scripts.push_back(s);
  processOrCollectScriptDeclarations(s);
}

Class ScriptCompiler::instantiate(const ClassTemplate & ct, const std::vector<TemplateArgument> & args, ScriptCompilerComponentKey)
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
    session()->generated.classes.push_back(result);

    StateGuard guard{ this };
    mCurrentScope = selected_specialization.first.argumentScope(selected_specialization.second);

    readClassContent(result, class_decl);

    return result;
  }
  else
  {
    ClassTemplateSpecializationBuilder builder = ClassTemplate{ ct }.Specialization(std::vector<TemplateArgument>{args});

    auto class_decl = ct.impl()->definition.get_class_decl();
    builder.name = readClassName(class_decl);
    builder.base = readClassBase(class_decl);

    Class result = builder.get();
    session()->generated.classes.push_back(result);

    StateGuard guard{ this };
    mCurrentScope = ct.argumentScope(args);

    readClassContent(result, class_decl);

    return result;
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
  scriptfunc->enclosing_symbol = mCurrentScript.impl();

  auto fakedecl = ast::FunctionDecl::New(std::shared_ptr<ast::AST>());
  fakedecl->body = ast::CompoundStatement::New(parser::Token{ parser::Token::LeftBrace, 0, 0, 0, 0 }, parser::Token{ parser::Token::RightBrace, 0, 0, 0, 0 });
  const auto & stmts = currentAst()->statements();
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
  for (const auto & decl : currentAst()->declarations())
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
  mProcessingQueue.push(ScopedDeclaration{ currentScope(), decl });
}

void ScriptCompiler::resolveIncompleteTypes()
{
  ScopeGuard guard{ mCurrentScope };

  //for (auto & f : mIncompleteFunctions)
  //  reprocess(f);

  //mIncompleteFunctions.clear();
  while (!mIncompleteFunctions.empty())
  {
    auto task = mIncompleteFunctions.front();
    mIncompleteFunctions.pop();
    reprocess(task);
  }
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



bool ScriptCompiler::compileFunctions()
{
  FunctionCompiler fcomp{ compiler() };

  /*for (size_t i(0); i < this->mCompilationTasks.size(); ++i)
  {
    const auto & task = this->mCompilationTasks.at(i);
    fcomp.compile(task);
  }*/

  while (!mCompilationTasks.empty())
  {
    auto task = mCompilationTasks.front();
    mCompilationTasks.pop();
    fcomp.compile(task);
  }

  return true;
}

void ScriptCompiler::processClassDeclaration(const std::shared_ptr<ast::ClassDecl> & class_decl)
{
  assert(class_decl != nullptr);

  ClassBuilder builder = currentScope().symbol().Class("");
  fill(builder, class_decl);

  Class cla = builder.get();
  mCurrentScope.invalidateCache();

  readClassContent(cla, class_decl);
}

std::string ScriptCompiler::readClassName(const std::shared_ptr<ast::ClassDecl> & decl)
{
  return decl->name->getName();
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

  Enum e = symbol.Enum(enum_decl.name->getName()).get();

  mCurrentScope.invalidateCache();

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

  Symbol s = currentScope().symbol();
  s.Typedef(t, name).create();

  mCurrentScope.invalidateCache();
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
  FunctionBuilder builder = scp.symbol().Function(fundecl->name->getName());
  function_processor_.fill(builder, fundecl, scp);
  Function function = builder.create();

  if (function.isVirtual() && !fundecl->virtualKeyword.isValid())
    log(diagnostic::warning() << diagnostic::pos(fundecl->pos().line, fundecl->pos().col)
      << "Function overriding base virtual member declared without virtual or override specifier");

  schedule(function, fundecl, scp);
}

void ScriptCompiler::processConstructorDeclaration(const std::shared_ptr<ast::ConstructorDecl> & decl)
{
  const Scope scp = currentScope();
  Class current_class = scp.asClass();

  FunctionBuilder b = current_class.Constructor();
  function_processor_.fill(b, decl, scp);
  Function ctor = b.create();

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
  Function dtor = b.create();
  
  schedule(dtor, decl, scp);
}

void ScriptCompiler::processLiteralOperatorDecl(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  Scope scp = currentScope().escapeTemplate();

  if (!scp.isNamespace())
    throw NotImplementedError{ dpos(decl), "Literal operator can only be declared inside a namespace" };

  std::string suffix_name = decl->name->as<ast::LiteralOperatorName>().suffix_string();

  FunctionBuilder b = scp.asNamespace().UserDefinedLiteral(suffix_name);
  function_processor_.fill(b, decl, currentScope());

  Function function = b.create();

  schedule(function, decl, scp);
}

void ScriptCompiler::processOperatorOverloadingDeclaration(const std::shared_ptr<ast::OperatorOverloadDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::OperatorOverloadDecl & over_decl = *decl;

  if (over_decl.name->is<ast::LiteralOperatorName>())
    return processLiteralOperatorDecl(decl);

  FunctionBuilder builder = scp.symbol().Operation(Operator::Null);
  function_processor_.fill(builder, decl, scp);
  
  const bool is_member = currentScope().isClass();
  
  auto arity = builder.proto.count() == 2 ? ast::OperatorName::BuiltInOpResol::BinaryOp : ast::OperatorName::UnaryOp;
  builder.operation = ast::OperatorName::getOperatorId(over_decl.name->name, arity);
  if (builder.operation == Operator::Null)
  {
    // operator++(int) and operator++() trick
    if ((over_decl.name->name == parser::Token::PlusPlus || over_decl.name->name == parser::Token::MinusMinus)
      && (builder.proto.count() == 2 && builder.proto.at(1) == Type::Int))
    {
      builder.proto.popParameter();
      builder.operation = over_decl.name->name == parser::Token::PlusPlus ? PostIncrementOperator : PostDecrementOperator;
    }
    else
      throw CouldNotResolveOperatorName{ dpos(over_decl) };
  }

  if (Operator::isBinary(builder.operation) && builder.proto.count() != 2)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), 2, builder.proto.count() };
  else if (Operator::isUnary(builder.operation) && builder.proto.count() != 1)
    throw InvalidParamCountInOperatorOverload{ dpos(over_decl), 1, builder.proto.count() };

  
  if (Operator::onlyAsMember(builder.operation) && !is_member)
    throw OpOverloadMustBeDeclaredAsMember{ dpos(over_decl) };

  Function function = builder.create();

  schedule(function, decl, scp);
}

void ScriptCompiler::processCastOperatorDeclaration(const std::shared_ptr<ast::CastDecl> & decl)
{
  const Scope scp = currentScope();
  const ast::CastDecl & cast_decl = *decl;

  const bool is_member = scp.isClass();
  assert(is_member); /// TODO : is this necessary (should be enforced by the parser)

  FunctionBuilder builder = scp.symbol().toClass().Conversion(Type::Null);
  function_processor_.fill(builder, decl, scp);

  Function cast = builder.create();
  
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
      throw NotImplementedError{ dpos(decl), "Invalid template parameter" };

    result.back().setDefaultValue(p.default_value);
  }

  return result;
}

void ScriptCompiler::processClassTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::ClassDecl> & classdecl)
{
  Scope scp = currentScope();

  std::string name = classdecl->name->getName();
  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  ClassTemplate ct = scp.symbol().ClassTemplate(std::move(name))
    .setParams(std::move(params))
    .setScope(scp)
    .setCallback(nullptr)
    .get();

  ct.impl()->definition = TemplateDefinition::make(decl);
}

void ScriptCompiler::processFunctionTemplateDeclaration(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  Scope scp = currentScope();

  std::string name = fundecl->name->getName();
  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  FunctionTemplate ft = scp.symbol().FunctionTemplate(std::move(name))
    .setParams(std::move(params))
    .setScope(scp)
    .deduce(nullptr).substitute(nullptr).instantiate(nullptr)
    .get();

  ft.impl()->definition = TemplateDefinition::make(decl);
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

  auto template_name = ast::Identifier::New(classdecl->name->name, classdecl->name->ast.lock());
  /// TODO : set a flag in the name resolver to prevent template instantiation and use the template name directly
  Namespace ns = findEnclosingNamespace(scp);
  ClassTemplate ct = findClassTemplate(classdecl->name->getName(), ns.templates());

  if (ct.isNull())
    throw CouldNotFindPrimaryClassTemplate{dpos(classdecl)};

  TemplateNameProcessor tnp; /// TODO : use a custom TNP
  auto template_full_name = std::static_pointer_cast<ast::TemplateIdentifier>(classdecl->name);
  std::vector<TemplateArgument> args = tnp.arguments(scp, template_full_name->arguments);

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

  auto template_name = ast::Identifier::New(classdecl->name->name, classdecl->name->ast.lock());
  /// TODO : set a flag in the name resolver to prevent template instantiation and use the template name directly
  Namespace ns = findEnclosingNamespace(scp);
  ClassTemplate ct = findClassTemplate(classdecl->name->getName(), ns.templates());

  if (ct.isNull())
    throw CouldNotFindPrimaryClassTemplate{ dpos(classdecl) };

  std::vector<TemplateParameter> params = processTemplateParameters(decl);

  auto ps = std::make_shared<PartialTemplateSpecializationImpl>(ct, std::move(params), scp, engine(), scp.symbol().impl());
  ps->definition = TemplateDefinition::make(decl);

  ct.impl()->specializations.push_back(PartialTemplateSpecialization{ ps });
}

void ScriptCompiler::processFunctionTemplateFullSpecialization(const std::shared_ptr<ast::TemplateDeclaration> & decl, const std::shared_ptr<ast::FunctionDecl> & fundecl)
{
  assert(decl->is_full_specialization());

  const Scope scp = currentScope();

  auto template_name = ast::Identifier::New(fundecl->name->name, fundecl->name->ast.lock());
  /// TODO : set a flag in the name resolver to prevent template instantiation and use the template name directly
  Namespace ns = findEnclosingNamespace(scp);
  ClassTemplate ct = findClassTemplate(fundecl->name->getName(), ns.templates());

  const std::vector<Template> & tmplts = ns.templates();

  if (tmplts.empty())
    throw CouldNotFindPrimaryFunctionTemplate{ dpos(fundecl) };

  std::vector<TemplateArgument> args;
  if (fundecl->name->is<ast::TemplateIdentifier>())
  {
    TemplateNameProcessor tnp; /// TODO : use a custom TNP
    auto template_full_name = std::static_pointer_cast<ast::TemplateIdentifier>(fundecl->name);
    args = tnp.arguments(scp, template_full_name->arguments);
  }

  FunctionBuilder builder{ Function::StandardFunction };
  function_processor_.fill(builder, fundecl, scp);
  /// TODO : to avoid this error, process all specializations at the end !
  if(function_processor_.prototype_.type_.relax)
    throw NotImplementedError{ dpos(fundecl), "Could not resolve some types in full specialization" };

  TemplateOverloadSelector selector;
  auto selection = selector.select(tmplts, args, builder.proto);

  if(selection.first.isNull())
    throw CouldNotFindPrimaryFunctionTemplate{ dpos(fundecl) };

  /// TODO : merge this duplicate of FunctionTemplateProcessor
  auto impl = std::make_shared<FunctionTemplateInstance>(selection.first, selection.second, builder.name, builder.proto, engine(), builder.flags);
  impl->implementation.callback = builder.callback;
  impl->data = builder.data;
  impl->enclosing_symbol = scp.symbol().impl();
  Function result = Function{ impl };

  schedule(result, fundecl, scp);

  /// TODO : should this be done now or after full compilation ?
  selection.first.impl()->instances[selection.second] = result;
}

void ScriptCompiler::reprocess(IncompleteFunction & func)
{
  const auto & decl = std::static_pointer_cast<ast::FunctionDecl>(func.declaration);
  mCurrentScope = func.scope;

  auto impl = func.function.impl();
  Prototype & proto = impl->prototype;
  if (proto.returnType().testFlag(Type::UnknownFlag))
    proto.setReturnType(resolve(decl->returnType));

  const int offset = proto.count() > 0 && proto.at(0).testFlag(Type::ThisFlag) ? 1 : 0;

  for (int i(offset); i < proto.count(); ++i)
  {
    if (proto.at(i).testFlag(Type::UnknownFlag))
      proto.setParameter(i, resolve(decl->params.at(i-offset).type));
  }

  default_arguments_.process(func.declaration->as<ast::FunctionDecl>().params, func.function, func.scope);
}

void ScriptCompiler::schedule(Function & f, const std::shared_ptr<ast::FunctionDecl> & fundecl, const Scope & scp)
{
  if (function_processor_.prototype_.type_.relax)
    mIncompleteFunctions.push(IncompleteFunction{ scp, fundecl, f });
  else
    default_arguments_.process(fundecl->params, f, scp);

  function_processor_.prototype_.type_.relax = false;

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

