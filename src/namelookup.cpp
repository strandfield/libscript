// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/namelookup.h"
#include "namelookup_p.h"
#include "scope_p.h"

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


static Scope find_scope(const std::shared_ptr<ast::Identifier> & name, const Scope & scope, const Scope & startScope)
{
  // this function returns a scope without any parent
  // rational : when looking for A::B::C
  // we don't want to find A::D in A::B

  if (scope.isNull())
    return Scope{};

  Scope result;


  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    const std::string & str = name->getName();
    result = scope.child(str);
  }
  else if (name->is<ast::OperatorName>())
    return Scope{};
  else if (name->is<ast::ScopedIdentifier>())
  {
    const auto & scpid = name->as<ast::ScopedIdentifier>();
    Scope leftScope = find_scope(scpid.lhs, scope, startScope);
    return find_scope(scpid.rhs, leftScope, startScope);
  }

  if (!result.isNull())
    return result;

  return find_scope(name, scope.parent(), startScope);
}

static void resolve_routine(Engine *e, const std::shared_ptr<ast::Identifier> & name, const Scope & s, const Scope & startScope, NameLookupImpl *result)
{
  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    s.lookup(name->getName(), result);
    return;
  }
  else if (name->is<ast::OperatorName>())
  {
    Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(name->as<ast::OperatorName>().name, ast::OperatorName::All);
    const auto & ops = s.lookup(op);
    result->functions.insert(result->functions.end(), ops.begin(), ops.end());
  }
  else if (name->is<ast::ScopedIdentifier>())
  {
    const auto & scpid = name->as<ast::ScopedIdentifier>();
    Scope leftScope = find_scope(scpid.lhs, s, startScope);
    return resolve_routine(e, scpid.rhs, leftScope, startScope, result);
  }
  
  if (!s.parent().isNull())
    resolve_routine(e, name, s.parent(), startScope, result);
}

NameLookupImpl::NameLookupImpl()
  : dataMemberIndex(-1)
  , globalIndex(-1)
  , localIndex(-1)
  , captureIndex(-1)
{

}

NameLookupImpl::NameLookupImpl(const Type & t)
  : dataMemberIndex(-1)
  , globalIndex(-1)
  , localIndex(-1)
  , captureIndex(-1)
{
  this->typeResult = t;
}

NameLookupImpl::NameLookupImpl(const Class & cla)
  : dataMemberIndex(-1)
  , globalIndex(-1)
  , localIndex(-1)
  , captureIndex(-1)
{
  this->typeResult = cla.id();
}

NameLookupImpl::NameLookupImpl(const Function & f)
  : dataMemberIndex(-1)
  , globalIndex(-1)
  , localIndex(-1)
  , captureIndex(-1)
{
  this->functions.push_back(f);
}


NameLookup::NameLookup(const std::shared_ptr<NameLookupImpl> & impl)
  : d(impl)
{

}

const Scope & NameLookup::scope() const
{
  return d->scope;
}

const std::string & NameLookup::name() const
{
  return d->name;
}


NameLookup::ResultType NameLookup::resultType() const
{
  if (!d->functions.empty())
    return FunctionName;
  else if (d->dataMemberIndex != -1)
    return DataMemberName;
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

NameLookup NameLookup::resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scope)
{
  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = scope;
  result->name = std::string{}; // we don't set any name in this overload, we do only so in a std::string overload

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    bool stop = true;
    switch (name->name.type)
    {
    case parser::Token::Void:
      result->typeResult = Type::Void;
      break;
    case parser::Token::Bool:
      result->typeResult = Type::Boolean;
      break;
    case parser::Token::Char:
      result->typeResult = Type::Char;
      break;
    case parser::Token::Int:
      result->typeResult = Type::Int;
      break;
    case parser::Token::Float:
      result->typeResult = Type::Float;
      break;
    case parser::Token::Double:
      result->typeResult = Type::Double;
      break;
    case parser::Token::Auto:
      result->typeResult = Type::Auto;
      break;
    default:
      stop = false;
      break;
    }

    if (stop)
      return NameLookup{ result };

    scope.lookup(name->getName(), result);
    return NameLookup{ result };
  }

  resolve_routine(scope.engine(), name, scope, scope, result.get());

  return NameLookup{ result };
}

NameLookup NameLookup::resolve(const std::string & name, const Scope & scope)
{
  if (name.find("::") != std::string::npos || name.find("<") != std::string::npos) /// TODO : use the parser
    throw std::runtime_error{ "Not implemented" }; 

  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = scope;
  result->name = name; 

  static const std::map<std::string, Type::BuiltInType> built_in_types = {
    { "void", Type::Void },
    { "bool", Type::Boolean },
    { "char", Type::Char },
    { "int", Type::Int },
    { "float", Type::Float },
    { "double", Type::Double },
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


static Function instantiate_function_template(const Template & t, std::vector<TemplateArgument> && args, const std::vector<Type> & input_types, compiler::AbstractExpressionCompiler *compiler)
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

static Function instantiate_function_template(const Template & t, const std::shared_ptr<ast::TemplateIdentifier> & name, const std::vector<Type> & input_types, compiler::AbstractExpressionCompiler *compiler)
{
  std::vector<TemplateArgument> args = compiler->generateTemplateArguments(name->arguments);
  return instantiate_function_template(t, std::move(args), input_types, compiler);
}


static NameLookup qualified_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & s, compiler::AbstractExpressionCompiler *compiler)
{
  assert(!name->is<ast::ScopedIdentifier>());

  ScopeParentGuard guard{ s };
  s.impl()->parent = nullptr; // temporarily setting parent to nullptr

  auto result = std::make_shared<NameLookupImpl>();

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    s.lookup(name->getName(), result);
  }
  else if (name->is<ast::OperatorName>())
  {
    Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(name->as<ast::OperatorName>().name, ast::OperatorName::All);
    const auto & ops = s.lookup(op);
    result->functions.insert(result->functions.end(), ops.begin(), ops.end());
  }
  else if (name->is<ast::TemplateIdentifier>())
  {
    const auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    auto fake_template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    NameLookup lookup = qualified_lookup(fake_template_name, s, compiler);
    if (lookup.templateResult().isNull())
      throw std::runtime_error{ "Name does not refer to a template" };
    Class cla = instantiate_class_template(lookup.templateResult(), tempid, compiler);
    result->typeResult = cla.id();
  }

  // we don't search in the scope' parent since this is a qualified lookup
  return NameLookup{ result };
}


static NameLookup qualified_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & s, const std::vector<Type> & input_types, compiler::AbstractExpressionCompiler *compiler)
{
  assert(!name->is<ast::ScopedIdentifier>());

  ScopeParentGuard guard{ s };
  s.impl()->parent = nullptr; // temporarily setting parent to nullptr

  auto result = std::make_shared<NameLookupImpl>();

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    const std::string & n = name->getName();
    s.lookup(n, result);


    if (!result->templateResult.isNull())
    {
      std::vector<TemplateArgument> template_args;
      Function f = instantiate_function_template(result->templateResult, std::move(template_args), input_types, compiler);
      if (!f.isNull())
      {
        result->templateResult = Template{};
        result->functions.push_back(f);
      }
    }
  }
  else if (name->is<ast::OperatorName>())
  {
    Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(name->as<ast::OperatorName>().name, ast::OperatorName::All);
    const auto & ops = s.lookup(op);
    result->functions.insert(result->functions.end(), ops.begin(), ops.end());
  }
  else if (name->is<ast::TemplateIdentifier>())
  {
    auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    auto template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    NameLookup tlookup = qualified_lookup(template_name, s, compiler);
    if (tlookup.resultType() != NameLookup::TemplateName)
      throw std::runtime_error{ "Not a template name" };

    Template t = tlookup.templateResult();
    Function f = instantiate_function_template(t, tempid, input_types, compiler);
    if (f.isNull())
      return NameLookup{ std::make_shared<NameLookupImpl>() };
    return NameLookup{ std::make_shared<NameLookupImpl>(f) };
  }

  // we don't search in the scope' parent since this is a qualified lookup
  return NameLookup{ result };
}


/// TODO : merge this function with the unqualified overload
// qualified scope lookup can be achieved with the unqualified version 
// because this function returns a Scope without any parent
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
    auto fake_template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    NameLookup lookup = qualified_lookup(fake_template_name, scope, compiler);
    if (lookup.templateResult().isNull())
      throw std::runtime_error{ "Name does not refer to a template" };
    return instantiate_class_template(lookup.templateResult(), tempid, compiler);
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
    auto fake_template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    NameLookup lookup = qualified_lookup(fake_template_name, scope, compiler);
    if(lookup.templateResult().isNull())
      throw std::runtime_error{ "Name does not refer to a template" };
    return instantiate_class_template(lookup.templateResult(), tempid, compiler);
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
  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    Type t;
    switch (name->name.type)
    {
    case parser::Token::Void:
      t = Type::Void;
      break;
    case parser::Token::Bool:
      t = Type::Boolean;
      break;
    case parser::Token::Char:
      t = Type::Char;
      break;
    case parser::Token::Int:
      t = Type::Int;
      break;
    case parser::Token::Float:
      t = Type::Float;
      break;
    case parser::Token::Double:
      t = Type::Double;
      break;
    case parser::Token::Auto:
      t = Type::Auto;
      break;
    default:
      break;
    }

    if (!t.isNull())
      return NameLookup{ std::make_shared<NameLookupImpl>(t) };
  }

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    return compiler->currentScope().lookup(name->getName());
  }
  else if (name->type() == ast::NodeType::QualifiedIdentifier)
  {
    auto qualid = std::dynamic_pointer_cast<ast::ScopedIdentifier>(name);
    Scope scp = unqualified_scope_lookup(qualid->lhs, compiler->currentScope(), compiler);
    return qualified_lookup(qualid->rhs, scp, compiler);
  }
  else if (name->type() == ast::NodeType::TemplateIdentifier)
  {
    auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    auto template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    NameLookup tlookup = compiler->currentScope().lookup(template_name->getName());
    if (tlookup.resultType() != NameLookup::TemplateName)
      throw std::runtime_error{ "Not a template name" };

    Template t = tlookup.templateResult();
    Class cla = instantiate_class_template(t, tempid, compiler);
    return NameLookup{ std::make_shared<NameLookupImpl>(cla) };
  }

  throw std::runtime_error{ "NameLookup::resolve() : not implemented" };
}


NameLookup NameLookup::resolve(const std::shared_ptr<ast::Identifier> & name, const std::vector<std::shared_ptr<program::Expression>> & args, compiler::AbstractExpressionCompiler *compiler)
{
  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    Type t;
    switch (name->name.type)
    {
    case parser::Token::Void:
      t = Type::Void;
      break;
    case parser::Token::Bool:
      t = Type::Boolean;
      break;
    case parser::Token::Char:
      t = Type::Char;
      break;
    case parser::Token::Int:
      t = Type::Int;
      break;
    case parser::Token::Float:
      t = Type::Float;
      break;
    case parser::Token::Double:
      t = Type::Double;
      break;
    case parser::Token::Auto:
      t = Type::Auto;
      break;
    default:
      break;
    }

    if (!t.isNull())
      return NameLookup{ std::make_shared<NameLookupImpl>(t) };
  }

  std::vector<Type> input_types;
  for (const auto & a : args)
    input_types.push_back(a->type());

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    NameLookup lookup = compiler->currentScope().lookup(name->getName());
    if (!lookup.templateResult().isNull())
    {
      std::vector<TemplateArgument> template_args;
      Function f = instantiate_function_template(lookup.templateResult(), std::move(template_args), input_types, compiler);
      if (!f.isNull())
      {
        lookup.impl()->templateResult = Template{};
        lookup.impl()->functions.push_back(f);
      }
    }
    return lookup;
  }
  else if (name->type() == ast::NodeType::QualifiedIdentifier)
  {
    auto qualid = std::dynamic_pointer_cast<ast::ScopedIdentifier>(name);
    Scope scp = unqualified_scope_lookup(qualid->lhs, compiler->currentScope(), compiler);
    return qualified_lookup(qualid->rhs, scp, input_types, compiler);
  }
  else if (name->type() == ast::NodeType::TemplateIdentifier)
  {
    auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    auto template_name = ast::Identifier::New(tempid->name, tempid->ast.lock());
    NameLookup tlookup = compiler->currentScope().lookup(template_name->getName());
    if (tlookup.resultType() != NameLookup::TemplateName)
      throw std::runtime_error{ "Not a template name" };

    Template t = tlookup.templateResult();
    Function f = instantiate_function_template(t, tempid, input_types, compiler);
    if (f.isNull())
      return NameLookup{ std::make_shared<NameLookupImpl>() };
    return NameLookup{ std::make_shared<NameLookupImpl>(f) };
  }
  
  throw std::runtime_error{ "Not implemented" };
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

} // namespace script
