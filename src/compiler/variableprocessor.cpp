// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/variableprocessor.h"

#include "script/compiler/diagnostichelper.h"

#include "script/array.h"
#include "../array_p.h"
#include "script/class.h"
#include "../class_p.h"
#include "script/engine.h"
#include "../engine_p.h"
#include "script/namespace.h"
#include "../namespace_p.h"
#include "script/object.h"

namespace script
{

namespace compiler
{

VariableProcessor::VariableProcessor(Engine *e)
  : engine_(e)
{

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
  else if (scp.type() == Scope::NamespaceScope)
    process_namespace_variable(decl, scp);
  else
    throw std::runtime_error{ "Not implemented" };
}

void VariableProcessor::initializeVariables()
{
  for (const auto & v : uninitialized_variables_)
    initialize(v);
}

void VariableProcessor::process_namespace_variable(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp)
{
  Engine *e = scp.engine();
  Namespace ns = scp.asNamespace();
  assert(!ns.isNull());

  if (decl->variable_type.type->name == parser::Token::Auto)
    throw GlobalVariablesCannotBeAuto{ dpos(decl) };

  Type var_type = type_.resolve(decl->variable_type, scp);

  if (decl->init == nullptr)
    throw GlobalVariablesMustBeInitialized{ dpos(decl) };

  if (decl->init->is<ast::ConstructorInitialization>() || decl->init->is<ast::BraceInitialization>())
    throw GlobalVariablesMustBeAssigned{ dpos(decl) };

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
    val = e->uninitialized(var_type);
    uninitialized_variables_.push_back(Variable{ val, decl, scp });
  }

  e->manage(val);
  ns.implementation()->variables[decl->name->getName()] = val;
}

void VariableProcessor::process_data_member(const std::shared_ptr<ast::VariableDecl> & decl, const Scope & scp)
{
  Class c = scp.asClass();
  Engine *e = c.engine();
  assert(!c.isNull());

  if (decl->variable_type.type->name == parser::Token::Auto)
    throw DataMemberCannotBeAuto{ dpos(decl) };

  Type var_type = type_.resolve(decl->variable_type, scp);

  if (decl->staticSpecifier.isValid())
  {
    if (decl->init == nullptr)
      throw MissingStaticInitialization{ dpos(decl) };

    if (decl->init->is<ast::ConstructorInitialization>() || decl->init->is<ast::BraceInitialization>())
      throw InvalidStaticInitialization{ dpos(decl) };

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
      Value staticMember = c.implementation()->add_uninitialized_static_data_member(decl->name->getName(), var_type, scp.accessibility());
      uninitialized_variables_.push_back(Variable{ staticMember, decl, scp });
    }
  }
  else
  {
    Class::DataMember dataMember{ var_type, decl->name->getName(), scp.accessibility() };
    c.implementation()->dataMembers.push_back(dataMember);
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
    try
    {
      Value val = v.variable;
      engine->initialize(val);
    }
    catch (...)
    {
      throw FailedToInitializeStaticVariable{};
    }
  }
  else
  {
    std::shared_ptr<program::Expression> initexpr = expr_.generateExpression(parsed_initexpr);

    if (initexpr->is<program::ConstructorCall>())
    {
      auto ctorcall = std::dynamic_pointer_cast<program::ConstructorCall>(initexpr);
      try
      {
        std::vector<Value> args;
        for (const auto & a : ctorcall->arguments)
          args.push_back(eval(a));
        Value val = v.variable;
        engine->emplace(val, ctorcall->constructor, args);
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
        Value val = v.variable;
        Value arg = eval(initexpr);
        engine->uninitialized_copy(arg, val);
      }
      catch (...)
      {
        throw FailedToInitializeStaticVariable{};
      }
    }
  }
}

Value VariableProcessor::eval(const std::shared_ptr<program::Expression> & e)
{
  return e->accept(*this);
}

Value VariableProcessor::manage(const Value & v)
{
  engine()->manage(v);
  return v;
}


Value VariableProcessor::visit(const program::ArrayExpression & ae)
{
  Array a = engine()->newArray(Engine::ArrayType{ ae.arrayType });
  auto aimpl = a.impl();
  aimpl->resize(ae.elements.size());
  for (size_t i(0); i < ae.elements.size(); ++i)
    aimpl->elements[i] = eval(ae.elements.at(i));
  Value ret = Value::fromArray(a);
  engine()->manage(ret);
  return ret;
}

Value VariableProcessor::visit(const program::BindExpression &)
{
  throw InvalidStaticInitialization{};
}

Value VariableProcessor::visit(const program::CaptureAccess &)
{
  throw InvalidStaticInitialization{};
}

Value VariableProcessor::visit(const program::CommaExpression &)
{
  throw InvalidStaticInitialization{};
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
  return manage(engine()->invoke(cc.constructor, args));
}

Value VariableProcessor::visit(const program::Copy & c)
{
  Value val{ eval(c.argument) };
  return manage(engine()->copy(val));
}

Value VariableProcessor::visit(const program::FetchGlobal &)
{
  throw InvalidStaticInitialization{};
}

Value VariableProcessor::visit(const program::FunctionCall & fc)
{
  std::vector<Value> args;
  for (const auto & a : fc.args)
    args.push_back(eval(a));
  Value ret = engine()->invoke(fc.callee, args);
  if (fc.callee.returnType().isReference())
    return ret;
  return manage(ret);
}

Value VariableProcessor::visit(const program::FunctionVariableCall & fvc)
{
  throw InvalidStaticInitialization{};
}

Value VariableProcessor::visit(const program::FundamentalConversion & fc)
{
  Value src = eval(fc.argument);
  Value ret = fundamental_conversion(src, fc.dest_type.baseType().data(), engine());
  engine()->manage(ret);
  return ret;
}

Value VariableProcessor::visit(const program::LambdaExpression & le)
{
  throw InvalidStaticInitialization{};
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
  Object obj = eval(ma.object).toObject();
  return obj.getAttribute(ma.offset);
}

Value VariableProcessor::visit(const program::StackValue &)
{
  throw InvalidStaticInitialization{};
}

Value VariableProcessor::visit(const program::VirtualCall &)
{
  throw InvalidStaticInitialization{};
}

} // namespace compiler

} // namespace script

