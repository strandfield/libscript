// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/namelookup.h"
#include "namelookup_p.h"

#include "script/engine.h"
#include "scope_p.h"
#include "script/functiontype.h"

#include "script/parser/parser.h"

#include "script/compiler/expressioncompiler.h"
#include "script/program/expression.h"

namespace script
{

class ScopeParentGuard
{
  std::shared_ptr<ScopeImpl> guarded_scope;
  std::shared_ptr<ScopeImpl> parent_value;
public:
  ScopeParentGuard(const Scope & s)
  {
    guarded_scope = s.impl();
    parent_value = guarded_scope->parent;
  }

  ~ScopeParentGuard()
  {
    guarded_scope->parent = parent_value;
  }
};


NameLookupImpl::NameLookupImpl()
  : dataMemberIndex(-1)
  , globalIndex(-1)
  , localIndex(-1)
  , captureIndex(-1)
  , arguments(nullptr)
  , compiler(nullptr)
{

}

NameLookupImpl::~NameLookupImpl()
{
  this->compiler = nullptr;
  this->arguments = nullptr;
}


NameLookup::NameLookup(const std::shared_ptr<NameLookupImpl> & impl)
  : d(impl)
{

}

const Scope & NameLookup::scope() const
{
  return d->scope;
}

const std::shared_ptr<ast::Identifier> & NameLookup::identifier() const
{
  return d->identifier;
}

bool NameLookup::hasArguments() const
{
  return d->arguments != nullptr;
}

const std::vector<std::shared_ptr<program::Expression>> & NameLookup::arguments() const
{
  return *(d->arguments);
}

NameLookup::ResultType NameLookup::resultType() const
{
  if (!d->functions.empty())
    return FunctionName;
  else if (d->dataMemberIndex != -1)
    return DataMemberName;
  else if (!d->staticDataMemberResult.isNull())
    return StaticDataMemberName;
  else if (d->enumValueResult.isValid())
    return EnumValueName;
  else if (d->globalIndex != -1)
    return GlobalName;
  else if (d->localIndex != -1)
    return LocalName;
  else if (d->captureIndex != -1)
    return CaptureName;
  else if (!d->namespaceResult.isNull())
    return NamespaceName;
  else if (!d->templateResult.isNull())
    return TemplateName;
  else if (!d->typeResult.isNull())
    return TypeName;
  else if (!d->valueResult.isNull())
    return VariableName;

  return UnknownName;
}


const std::vector<Function> & NameLookup::functions() const
{
  return d->functions;
}

const Type & NameLookup::typeResult() const
{
  return d->typeResult;
}

const Value & NameLookup::variable() const
{
  return d->valueResult;
}

const Template & NameLookup::templateResult() const
{
  return d->templateResult;
}

int NameLookup::captureIndex() const
{
  return d->captureIndex;
}

int NameLookup::dataMemberIndex() const
{
  return d->dataMemberIndex;
}

int NameLookup::globalIndex() const
{
  return d->globalIndex;
}

int NameLookup::localIndex() const
{
  return d->localIndex;
}

const EnumValue & NameLookup::enumValueResult() const
{
  return d->enumValueResult;
}

const Namespace & NameLookup::namespaceResult() const
{
  return d->namespaceResult;
}

const Class::StaticDataMember & NameLookup::staticDataMemberResult() const
{
  return d->staticDataMemberResult;
}

const Class & NameLookup::memberOf() const
{
  return d->memberOfResult;
}

NameLookup NameLookup::resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scope)
{
  auto result = std::make_shared<NameLookupImpl>();
  result->identifier = name;
  result->scope = scope;

  NameLookup l{ result };
  l.process();
  return l;
}

static bool need_parse(const std::string & name)
{
  for (const char & c : name)
  {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
    if (!ok)
      return true;
  }

  return false;
}

NameLookup NameLookup::resolve(const std::string & name, const Scope & scope)
{
  if (need_parse(name))
  {
    /// TODO : make this whole block less ugly !!

    auto source = SourceFile::fromString(name);
    auto pdata = std::make_shared<parser::ParserData>(source);
    pdata->mAst = std::make_shared<ast::AST>(source);
    parser::ScriptFragment fragment{ pdata };
    parser::IdentifierParser idp{ &fragment };
    std::shared_ptr<ast::Identifier> id;
    try
    {
      id = idp.parse();
    }
    catch (...)
    {

    }

    if (!pdata->atEnd() || id == nullptr)
      throw std::runtime_error{ "Could not fully parse identifier" };

    auto result = NameLookup::resolve(id, scope);
    // the AST is going to die here so the identifier will no longer be valid
    result.impl()->identifier = nullptr;
    return result;
  }

  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = scope;

  static const std::map<std::string, Type::BuiltInType> built_in_types = {
    { "void", Type::Void },
    { "bool", Type::Boolean },
    { "char", Type::Char },
    { "int", Type::Int },
    { "float", Type::Float },
    { "double", Type::Double },
    { "auto", Type::Auto },
  };

  auto it = built_in_types.find(name);
  if (it != built_in_types.end())
  {
    result->typeResult = it->second;
    return NameLookup{ result };
  }

  scope.lookup(name, result);

  return NameLookup{ result };
}

NameLookup NameLookup::resolve(Operator::BuiltInOperator op, const Scope & scope)
{
  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = scope;

  result->functions = scope.lookup(op);

  return NameLookup{ result };
}



static Class instantiate_class_template(const Template & t, const std::shared_ptr<ast::TemplateIdentifier> & name, compiler::AbstractExpressionCompiler *compiler)
{
  if (t.isClassTemplate())
    throw std::runtime_error{ "Name does not refer to a class template" };

  ClassTemplate ct = t.asClassTemplate();
  std::vector<TemplateArgument> targs = compiler->generateTemplateArguments(name->arguments);
  Class instantiated = ct.getInstance(targs);
  return instantiated;
}


static Function instantiate_function_template_procedure(const Template & t, std::vector<TemplateArgument> && args, const std::vector<Type> & input_types, compiler::AbstractExpressionCompiler *compiler)
{
  if (t.isClassTemplate())
    throw std::runtime_error{ "Name does not refer to a function template" };

  assert(t.isFunctionTemplate());
  FunctionTemplate ft = t.asFunctionTemplate();
  bool success = ft.deduce(args, input_types);
  if (!success)
    return Function{};
  Function instantiated = ft.getInstance(args);
  return instantiated;
}

static Function instantiate_function_template_procedure(const Template & t, const std::shared_ptr<ast::TemplateIdentifier> & name, const std::vector<Type> & input_types, compiler::AbstractExpressionCompiler *compiler)
{
  std::vector<TemplateArgument> args = compiler->generateTemplateArguments(name->arguments);
  return instantiate_function_template_procedure(t, std::move(args), input_types, compiler);
}

static Template qualified_template_lookup(const std::string & name, const Scope & scp)
{
  const auto & tmplts = scp.templates();
  for (const auto & t : tmplts)
  {
    if (t.name() == name)
      return t;
  }

  return Template{};
}

/// TODO : merge this function with the unqualified overload
static Scope qualified_scope_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & scope, compiler::AbstractExpressionCompiler *compiler)
{
  assert(!name->is<ast::ScopedIdentifier>());


  if (scope.isNull())
    return Scope{};

  Scope result;

  if (name->is<ast::OperatorName>())
    throw std::runtime_error{ "NameLookup error : an operator cannot be used as a scope" };

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    const std::string & str = name->getName();
    result = scope.child(str);
  }
  else if (name->is<ast::TemplateIdentifier>())
  {
    const auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    Template t = qualified_template_lookup(tempid->getName(), scope);
    if (t.isNull())
      throw std::runtime_error{ "Name does not refer to a template" };
    return instantiate_class_template(t, tempid, compiler);
  }

  return result;
}

static Scope unqualified_scope_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & scope, compiler::AbstractExpressionCompiler *compiler)
{
  if (scope.isNull())
    return Scope{};

  Scope result;

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    const std::string & str = name->getName();
    result = scope.child(str);
  }
  else if (name->is<ast::OperatorName>())
    throw std::runtime_error{ "NameLookup error : an operator cannot be used as a scope" };
  else if (name->is<ast::TemplateIdentifier>())
  {
    const auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    Template t = qualified_template_lookup(tempid->getName(), scope);
    if (t.isNull())
      throw std::runtime_error{ "Name does not refer to a template" };
    return instantiate_class_template(t, tempid, compiler);
  }
  else if (name->is<ast::ScopedIdentifier>())
  {
    const auto & scpid = name->as<ast::ScopedIdentifier>();
    Scope leftScope = unqualified_scope_lookup(scpid.lhs, scope, compiler);
    if(!leftScope.isNull())
      return qualified_scope_lookup(scpid.rhs, leftScope, compiler);
  }

  if (!result.isNull())
    return result;

  return unqualified_scope_lookup(name, scope.parent(), compiler);
}
 
NameLookup NameLookup::resolve(const std::shared_ptr<ast::Identifier> & name, compiler::AbstractExpressionCompiler *compiler)
{
  auto result = std::make_shared<NameLookupImpl>();
  result->identifier = name;
  result->compiler = compiler;
  result->scope = compiler->scope();

  NameLookup l{ result };
  l.process();
  return l;
}


NameLookup NameLookup::resolve(const std::shared_ptr<ast::Identifier> & name, const std::vector<std::shared_ptr<program::Expression>> & args, compiler::AbstractExpressionCompiler *compiler)
{
  auto result = std::make_shared<NameLookupImpl>();
  result->identifier = name;
  result->arguments = &args;
  result->compiler = compiler;
  result->scope = compiler->scope();

  NameLookup l{ result };
  l.process();
  return l;
}


NameLookup NameLookup::member(const std::string & name, const Class & cla)
{
  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = Scope{ cla };

  for (const auto & f : cla.memberFunctions())
  {
    if (f.name() == name)
      result->functions.push_back(f);
  }

  /// TODO : implement using-directive for functions

  /// TODO : look for function templates

  const auto & data_members = cla.dataMembers();
  for (size_t i(0); i < data_members.size(); ++i)
  {
    if (data_members.at(i).name == name)
    {
      result->dataMemberIndex = i + cla.attributesOffset();
      return NameLookup{ result };
    }
  }
  
  if(result->functions.empty())
    return NameLookup{ result };

  Class base = cla.parent();
  if (base.isNull())
    return NameLookup{ result };

  return NameLookup::member(name, base);
}

static void remove_duplicated_operators(std::vector<Function> & list)
{
  /// TODO : quicksort and remove duplicate
  // note that this is not required for OverloadResolution to work
  // we need to check if that is faster to remove the duplicates or not
}

static void get_scope_operators(std::vector<Function> & list, Operator::BuiltInOperator op, const script::Scope & scp, int opts)
{
  const auto & candidates = scp.operators();
  for (const auto & c : candidates)
  {
    if (c.operatorId() != op)
      continue;
    list.push_back(c);
  }

  if ((opts & OperatorLookup::FetchParentOperators) && !scp.parent().isNull())
    get_scope_operators(list, op, scp.parent(), OperatorLookup::FetchParentOperators);

  if (opts & OperatorLookup::RemoveDuplicates)
    return remove_duplicated_operators(list);
}


static void resolve_operators(std::vector<Function> &result, Operator::BuiltInOperator op, const Type & type, const Scope & scp, int opts)
{
  Engine *engine = scp.engine();

  if (type.isClosureType() || type.isFunctionType())
  {
    // these two don't have a definition scope, so we must process them separatly
    if (type.isFunctionType() && op == Operator::AssignmentOperator)
    {
      result.push_back(engine->getFunctionType(type).assignment());
      return;
    }
    else if (type.isClosureType() && op == Operator::FunctionCallOperator)
    {
      result.push_back(engine->getLambda(type).function());
      return;
    }

    return;
  }

  script::Scope type_decl_scope = engine->scope(type);
  if (type.isObjectType())
  {
    script::Scope class_scope = script::Scope{ engine->getClass(type), type_decl_scope };
    get_scope_operators(result, op, class_scope, OperatorLookup::FetchParentOperators);

    Class parent = class_scope.asClass().parent();
    if (!parent.isNull())
      resolve_operators(result, op, parent.id(), scp, OperatorLookup::FetchParentOperators);
  }
  else
  {
    get_scope_operators(result, op, type_decl_scope, OperatorLookup::FetchParentOperators);
  }

  if (type.isEnumType() && op == Operator::AssignmentOperator)
    result.push_back(engine->getEnum(type).getAssignmentOperator());

  if (opts & OperatorLookup::ConsiderCurrentScope)
  {
    /// TODO : check if type_decl_scope == scp to avoid unesseccary operation

    get_scope_operators(result, op, scp, OperatorLookup::FetchParentOperators);
  }

  if (opts & OperatorLookup::RemoveDuplicates)
    remove_duplicated_operators(result);
}

std::vector<Function> NameLookup::resolve(Operator::BuiltInOperator op, const Type & type, const Scope & scp, int opts)
{
  std::vector<Function> result;
  resolve_operators(result, op, type, scp, opts);
  return result;
}

std::vector<Function> NameLookup::resolve(Operator::BuiltInOperator op, const Type & lhs, const Type & rhs, const Scope & scp, int opts)
{
  /// TODO : this needs some optimization !!
  std::vector<Function> result;
  resolve_operators(result, op, lhs, scp, opts);
  resolve_operators(result, op, rhs, scp, opts & ~OperatorLookup::ConsiderCurrentScope);
  return result;
}

compiler::AbstractExpressionCompiler * NameLookup::getCompiler()
{
  if (d->compiler != nullptr)
    return d->compiler;

  d->default_compiler_compiler = std::unique_ptr<compiler::Compiler>(new compiler::Compiler{ scope().engine() });
  auto *c = new compiler::ExpressionCompiler{ d->default_compiler_compiler.get(), d->default_compiler_compiler->session() };
  c->setScope(d->scope);
  d->default_compiler = std::unique_ptr<compiler::AbstractExpressionCompiler>(c);
  d->compiler = d->default_compiler.get();
  return d->compiler;
}

bool NameLookup::checkBuiltinName()
{
  if (d->identifier->type() != ast::NodeType::SimpleIdentifier)
    return false;

  switch (d->identifier->name.type)
  {
  case parser::Token::Void:
    d->typeResult = Type::Void;
    return true;
  case parser::Token::Bool:
    d->typeResult = Type::Boolean;
    return true;
  case parser::Token::Char:
    d->typeResult = Type::Char;
    return true;
  case parser::Token::Int:
    d->typeResult = Type::Int;
    return true;
  case parser::Token::Float:
    d->typeResult = Type::Float;
    return true;
  case parser::Token::Double:
    d->typeResult = Type::Double;
    return true;
  case parser::Token::Auto:
    d->typeResult = Type::Auto;
    return true;
  default:
    break;
  }

  return false;
}

void NameLookup::process()
{
  if (checkBuiltinName())
    return;

  auto name = d->identifier;
  auto compiler = getCompiler();
  auto scope = d->scope;

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    scope.lookup(name->getName(), d.get());
    instantiate_function_template(name);
  }
  else if (name->is<ast::OperatorName>())
  {
    Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(name->as<ast::OperatorName>().name, ast::OperatorName::All);
    const auto & ops = scope.lookup(op);
    d->functions.insert(d->functions.end(), ops.begin(), ops.end());
  }
  else if (name->type() == ast::NodeType::QualifiedIdentifier)
  {
    auto qualid = std::dynamic_pointer_cast<ast::ScopedIdentifier>(name);
    Scope scp = unqualified_scope_lookup(qualid->lhs, compiler->scope(), compiler);
    return qualified_lookup(qualid->rhs, scp);
  }
  else if (name->type() == ast::NodeType::TemplateIdentifier)
  {
    auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    auto template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    NameLookup tlookup = compiler->scope().lookup(template_name->getName());
    if (tlookup.resultType() != NameLookup::TemplateName)
      throw std::runtime_error{ "Not a template name" };

    Template t = tlookup.templateResult();
    Class cla = instantiate_class_template(t, tempid, compiler);
    d->typeResult = cla.id();
  }
}

void NameLookup::qualified_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & s)
{
  assert(!name->is<ast::ScopedIdentifier>());

  if (s.isNull())
    return;

  ScopeParentGuard guard{ s };
  s.impl()->parent = nullptr; // temporarily setting parent to nullptr

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    s.lookup(name->getName(), d.get());
    instantiate_function_template(name);
  }
  else if (name->is<ast::OperatorName>())
  {
    Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(name->as<ast::OperatorName>().name, ast::OperatorName::All);
    const auto & ops = s.lookup(op);
    d->functions.insert(d->functions.end(), ops.begin(), ops.end());
  }
  else if (name->is<ast::TemplateIdentifier>())
  {
    const auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    auto fake_template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    qualified_lookup(fake_template_name, s);
    if (d->templateResult.isNull())
      throw std::runtime_error{ "Name does not refer to a template" };

    if (d->templateResult.isClassTemplate())
    {
      Class cla = instantiate_class_template(d->templateResult, tempid, getCompiler());
      d->templateResult = Template{};
      d->typeResult = cla.id();
    }
    else if (d->templateResult.isFunctionTemplate())
    {
      instantiate_function_template(tempid);
    }
  }
}

void NameLookup::instantiate_function_template(const std::shared_ptr<ast::Identifier> & name)
{
  if (!hasArguments())
    return;

  if (d->templateResult.isNull())
    return;

  std::vector<Type> input_types;
  for (const auto & a : arguments())
    input_types.push_back(a->type());

  std::vector<TemplateArgument> template_args;
  if (name->is<ast::TemplateIdentifier>())
  {
    template_args = getCompiler()->generateTemplateArguments(name->as<ast::TemplateIdentifier>().arguments);
  }

  Function f = instantiate_function_template_procedure(d->templateResult, std::move(template_args), input_types, getCompiler());
  if (!f.isNull())
  {
    d->templateResult = Template{};
    d->functions.push_back(f);
  }
}

} // namespace script
