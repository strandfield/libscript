// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/variableprocessor.h"

#include "script/compiler/diagnostichelper.h"
#include "script/compiler/compilererrors.h"

#include "script/array.h"
#include "script/class.h"
#include "script/engine.h"
#include "script/enumerator.h"
#include "script/namespace.h"
#include "script/object.h"
#include "script/typesystem.h"

#include "script/private/array_p.h"
#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/namespace_p.h"
#include "script/private/value_p.h"

namespace script
{

namespace compiler
{

VariableProcessor::VariableProcessor(Compiler *c)
  : expr_(c)
{
  engine_ = expr_.engine();
}

void VariableProcessor::process(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp)
{
  if (scp.type() == Scope::ScriptScope)
  {
    // Global variables are processed by the function compiler
    return;
  }

  if (scp.isClass())
    process_data_member(decl, scp);
  else if (scp.isNamespace())
    process_namespace_variable(decl, scp);
  else
    throw std::runtime_error{ "Not implemented" };
}

void VariableProcessor::initializeVariables()
{
  for (const auto & v : uninitialized_variables_)
    initialize(v);

  uninitialized_variables_.clear();
}

void VariableProcessor::process_namespace_variable(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp)
{
  Engine *e = scp.engine();
  Namespace ns = scp.asNamespace();
  assert(!ns.isNull());

  if (decl->variable_type.type->is<ast::SimpleIdentifier>() && decl->variable_type.type->as<ast::SimpleIdentifier>().name == parser::Token::Auto)
    throw CompilationFailure{ CompilerError::GlobalVariablesCannotBeAuto };

  Type var_type = script::compiler::resolve_type(decl->variable_type, scp);

  if (decl->init == nullptr)
    throw CompilationFailure{ CompilerError::GlobalVariablesMustBeInitialized };

  if (decl->init->is<ast::ConstructorInitialization>() || decl->init->is<ast::BraceInitialization>())
    throw CompilationFailure{ CompilerError::GlobalVariablesMustBeAssigned };

  auto expr = decl->init->as<ast::AssignmentInitialization>().value;
  Value val;
  if (var_type.isFundamentalType() && expr->is<ast::Literal>() && !expr->is<ast::UserDefinedLiteral>())
  {
    expr_.setScope(scp);
    auto execexpr = expr_.generateExpression(expr);
    val = e->implementation()->interpreter->eval(execexpr);
  }
  else
  {
    val = e->construct(var_type, {});
    uninitialized_variables_.push_back(Variable{ val, decl, scp });
  }

  ns.impl()->variables[decl->name->getName()] = val;
}

void VariableProcessor::process_data_member(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp)
{
  Class c = scp.asClass();
  Engine *e = c.engine();
  assert(!c.isNull());

  if (decl->variable_type.type->is<ast::SimpleIdentifier>() && decl->variable_type.type->as<ast::SimpleIdentifier>().name == parser::Token::Auto)
    throw CompilationFailure{ CompilerError::DataMemberCannotBeAuto };

  Type var_type = script::compiler::resolve_type(decl->variable_type, scp);

  if (decl->staticSpecifier.isValid())
  {
    if (decl->init == nullptr)
      throw CompilationFailure{ CompilerError::MissingStaticInitialization };

    if (decl->init->is<ast::ConstructorInitialization>() || decl->init->is<ast::BraceInitialization>())
      throw CompilationFailure{ CompilerError::InvalidStaticInitialization };

    auto expr = decl->init->as<ast::AssignmentInitialization>().value;
    if (var_type.isFundamentalType() && expr->is<ast::Literal>() && !expr->is<ast::UserDefinedLiteral>())
    {
      expr_.setScope(scp);
      auto execexpr = expr_.generateExpression(expr);

      Value val = e->implementation()->interpreter->eval(execexpr);
      c.addStaticDataMember(decl->name->getName(), val, scp.accessibility());
    }
    else
    {
      Value staticMember = c.impl()->add_default_constructed_static_data_member(decl->name->getName(), var_type, scp.accessibility());
      uninitialized_variables_.push_back(Variable{ staticMember, decl, scp });
    }
  }
  else
  {
    Class::DataMember dataMember{ var_type, decl->name->getName(), scp.accessibility() };
    c.impl()->dataMembers.push_back(dataMember);
  }
}

void VariableProcessor::initialize(Variable v)
{
  Engine *engine = v.scope.engine();

  expr_.setScope(v.scope);

  const auto & init = v.declaration->init;
  auto parsed_initexpr = init->as<ast::AssignmentInitialization>().value;
  if (parsed_initexpr == nullptr)
  {
    default_initialization(v);
  }
  else
  {
    std::shared_ptr<program::Expression> initexpr = expr_.generateExpression(parsed_initexpr);
    copy_initialization(v, initexpr);
  }
}

void VariableProcessor::default_initialization(Variable & v)
{
  Engine *engine = v.scope.engine();

  Value val = v.variable;
  if (val.type().isFundamentalType())
    return;
  else if (val.type().isEnumType())
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };
  else if (val.type().isFunctionType() || val.type().isClosureType())
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };

  assert(val.type().isObjectType());

  Function ctor = engine->typeSystem()->getClass(val.type()).defaultConstructor();
  if (ctor.isNull())
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };

  try
  {
    ctor.invoke({ val });
  }
  catch (...)
  {
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };
  }
}

void VariableProcessor::copy_initialization(Variable & var, const std::shared_ptr<program::Expression> & value)
{
  if (value->is<program::ConstructorCall>())
    return constructor_initialization(var, std::static_pointer_cast<program::ConstructorCall>(value));

  Engine *engine = var.scope.engine();

  Value arg = eval(value);
  if(var.variable.type() != arg.type())
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };

  const Type t = arg.type();

  if (t.isFundamentalType())
  {
    switch (t.baseType().data())
    {
    case Type::Boolean:
      script::get<bool>(var.variable) = arg.toBool();
      break;
    case Type::Char:
      script::get<char>(var.variable) = arg.toChar();
      break;
    case Type::Int:
      script::get<int>(var.variable) = arg.toInt();
      break;
    case Type::Float:
      script::get<float>(var.variable) = arg.toFloat();
      break;
    case Type::Double:
      script::get<double>(var.variable) = arg.toDouble();
      break;
    default:
      break;
    }
  }
  else if (t.isEnumType())
  {
    script::get<Enumerator>(var.variable) = arg.toEnumerator();
  }
  else if (t.isFunctionType())
  {
    script::get<Function>(var.variable) = arg.toFunction();
  }
  else if (t.isObjectType())
  {
    Function copy_ctor = engine->typeSystem()->getClass(t).copyConstructor();
    if(copy_ctor.isNull())
      throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };

    // @TODO: perform assignment
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };
    copy_ctor.invoke({ var.variable, arg });
  }
  else
  {
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };
  }
}

void VariableProcessor::constructor_initialization(Variable & var, const std::shared_ptr<program::ConstructorCall> & call)
{
  Engine *engine = var.scope.engine();

  try
  {
    std::vector<Value> args;
    args.push_back(var.variable);
    for (const auto & a : call->arguments)
      args.push_back(eval(a));
    call->constructor.invoke(args);
  }
  catch (...)
  {
    throw CompilationFailure{ CompilerError::FailedToInitializeStaticVariable };
  }
}

Value VariableProcessor::eval(const std::shared_ptr<program::Expression> & e)
{
  return e->accept(*this);
}

Value VariableProcessor::manage(const Value & v)
{
  return v;
}


Value VariableProcessor::visit(const program::ArrayExpression & ae)
{
  Array a = engine()->newArray(Engine::ArrayType{ ae.arrayType });
  auto aimpl = a.impl();
  aimpl->resize(static_cast<int>(ae.elements.size()));
  for (size_t i(0); i < ae.elements.size(); ++i)
    aimpl->elements[i] = eval(ae.elements.at(i));
  Value ret = Value::fromArray(a);
  return ret;
}

Value VariableProcessor::visit(const program::BindExpression &)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::CaptureAccess &)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::CommaExpression &)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::ConditionalExpression & ce)
{
  if (eval(ce.cond).toBool())
    return eval(ce.onTrue);
  return eval(ce.onFalse);
}

Value VariableProcessor::visit(const program::ConstructorCall & cc)
{
  std::vector<Value> args;
  for (const auto & a : cc.arguments)
    args.push_back(eval(a));
  // TODO: check correctness
  Value result = args.front();
  cc.constructor.invoke(args);
  return manage(result);
}

Value VariableProcessor::visit(const program::Copy & c)
{
  Value val{ eval(c.argument) };
  return manage(engine()->copy(val));
}

Value VariableProcessor::visit(const program::FetchGlobal &)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::FunctionCall & fc)
{
  std::vector<Value> args;
  for (const auto & a : fc.args)
    args.push_back(eval(a));
  Value ret = fc.callee.invoke(args);
  if (fc.callee.returnType().isReference())
    return ret;
  return manage(ret);
}

Value VariableProcessor::visit(const program::FunctionVariableCall & fvc)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::FundamentalConversion & fc)
{
  Value src = eval(fc.argument);
  Value ret = fundamental_conversion(src, fc.dest_type.baseType().data(), engine());
  return ret;
}

Value VariableProcessor::visit(const program::InitializerList &)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::LambdaExpression & le)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::Literal & l)
{
  return l.value;
}

Value VariableProcessor::visit(const program::LogicalAnd & la)
{
  Value lhs = eval(la.lhs);
  if (!lhs.toBool())
    return lhs;
  return eval(la.rhs);
}

Value VariableProcessor::visit(const program::LogicalOr & lo)
{
  Value lhs = eval(lo.lhs);
  if (lhs.toBool())
    return lhs;
  return eval(lo.rhs);
}

Value VariableProcessor::visit(const program::MemberAccess & ma)
{
  Value object = eval(ma.object);
  return object.impl()->at(ma.offset);
}

Value VariableProcessor::visit(const program::StackValue &)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

Value VariableProcessor::visit(const program::VariableAccess & va)
{
  /// TODO : detect circular references during initialization
  return va.value;
}

Value VariableProcessor::visit(const program::VirtualCall &)
{
  throw CompilationFailure{ CompilerError::InvalidStaticInitialization };
}

} // namespace compiler

} // namespace script

