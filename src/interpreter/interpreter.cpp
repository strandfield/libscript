// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/interpreter/interpreter.h"

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/private/array_p.h"
#include "script/context.h"
#include "script/private/function_p.h"
#include "script/private/lambda_p.h"
#include "script/object.h"
#include "script/private/object_p.h"
#include "script/private/script_p.h"
#include "script/private/value_p.h"

namespace script
{

namespace interpreter
{

Interpreter::Interpreter(std::shared_ptr<ExecutionContext> ec, Engine *e)
  : mEngine(e)
  , mExecutionContext(ec)
{

}

Interpreter::~Interpreter()
{
  // Interesting note: we cannot make this destructor = default
  // because in the header, the InterpreterImpl type is not defined 
  // and thus its destructor is not available (causes a compile error).
}

Value Interpreter::call(const Function & f, const Value *obj, const Value *begin, const Value *end)
{
  Engine *e = f.engine();
  const int sp = mExecutionContext->stack.size;

  if (f.isConstructor())
  {
    assert(obj == nullptr);
    mExecutionContext->stack.push(createObject(f.returnType()));
  }
  else if (f.isDestructor());
  else
    mExecutionContext->stack.push(Value{});

  if (obj)
    mExecutionContext->stack.push(*obj);
  
  const Prototype & proto = f.prototype();
  const int argc = std::distance(begin, end);

  try
  {
    for (int i(0); i < argc; ++i)
    {
      Value arg = e->cast(begin[i], proto.argv(i));
      if (!proto.argv(i).isReference())
        e->manage(arg);
      mExecutionContext->stack.push(arg);
    }
  }
  catch (const std::runtime_error & exception)
  {
    // pop the arguments
    while (mExecutionContext->stack.size > sp + 1)
      mExecutionContext->stack.pop();

    mExecutionContext->stack.pop();

    throw exception;
  }

  mExecutionContext->push(f, sp);

  invoke(f);

  return mExecutionContext->pop();
}

Value Interpreter::call(const Function & f, const std::vector<Value> & args)
{
  Engine *e = f.engine();
  const int sp = mExecutionContext->stack.size;

  if (f.isConstructor())
    mExecutionContext->stack.push(createObject(f.returnType()));
  else if (f.isDestructor());
  else
    mExecutionContext->stack.push(Value{});

  const Prototype & proto = f.prototype();
  const int argc = args.size();

  try
  {
    for (int i(0); i < argc; ++i)
    {
      Value arg = e->cast(args[i], proto.argv(i));
      if (!proto.argv(i).isReference())
        e->manage(arg);
      mExecutionContext->stack.push(arg);
    }
  }
  catch (const std::runtime_error & exception)
  {
    // pop the arguments
    while (mExecutionContext->stack.size > sp + 1)
      mExecutionContext->stack.pop();

    throw exception;
  }

  mExecutionContext->push(f, sp);

  invoke(f);

  return mExecutionContext->pop();
}

Value Interpreter::invoke(const Function & f, const Value *obj, const Value *begin, const Value *end)
{
  const int sp = mExecutionContext->stack.size;
  
  if (f.isConstructor())
  {
    assert(obj == nullptr);
    mExecutionContext->stack.push(createObject(f.returnType()));
  }
  else if (f.isDestructor());
  else
    mExecutionContext->stack.push(Value{});

  if (obj)
    mExecutionContext->stack.push(*obj);
  for (auto it = begin; it != end; ++it)
    mExecutionContext->stack.push(*it);

  mExecutionContext->push(f, sp);

  invoke(f);

  return mExecutionContext->pop();
}

Value Interpreter::eval(program::Expression & expr)
{
  return expr.accept(*this);
}

Value Interpreter::eval(const std::shared_ptr<program::Expression> & expr)
{
  return expr->accept(*this);
}

void Interpreter::exec(program::Statement & s)
{
  s.accept(*this);
}

void Interpreter::exec(const std::shared_ptr<program::Statement> & s)
{
  s->accept(*this);
}


void Interpreter::invoke(const Function & f)
{
  auto impl = f.implementation();
  if (f.isNative()) {
    interpreter::FunctionCall *fcall = mExecutionContext->callstack.top();
    fcall->setReturnValue(impl->implementation.callback(fcall));
  } else {
    exec(impl->implementation.program);
  }

  mEngine->garbageCollect();
}

Value Interpreter::createObject(const Type & t)
{
  return mEngine->implementation()->buildValue(t.baseType());
}

void Interpreter::visit(const program::BreakStatement & bs) 
{
  for (const auto & s : bs.destruction)
    exec(s);

  mExecutionContext->callstack.top()->setBreakFlag();
}

void Interpreter::visit(const program::CompoundStatement & cs) 
{
  for (const auto & s : cs.statements)
  {
    exec(s);

    auto flags = mExecutionContext->flags();
    if (flags == FunctionCall::ReturnFlag || flags == FunctionCall::ContinueFlag || flags == FunctionCall::BreakFlag)
      return;
  }
}

void Interpreter::visit(const program::ContinueStatement & cs) 
{
  for (const auto & s : cs.destruction)
    exec(s);

  mExecutionContext->callstack.top()->setContinueFlag();
}

void Interpreter::visit(const program::InitObjectStatement & cos)
{
  Value & memplace = mExecutionContext->callstack.top()->returnValue();
  memplace.impl()->init_object();
}

void Interpreter::visit(const program::ExpressionStatement & es) 
{
  // TODO : handle destruction
  eval(es.expr);
}

void Interpreter::visit(const program::ForLoop & fl) 
{
  exec(fl.init);
  while (eval(fl.cond).toBool())
  {
    exec(fl.body);

    auto flags = mExecutionContext->flags();
    if (flags == FunctionCall::ReturnFlag)
      return;
    mExecutionContext->clearFlags();
    if (flags == FunctionCall::BreakFlag)
      return; // the break statement should have destroyed the variable in the init-scope.

    eval(fl.loop);
  }

  exec(fl.destroy);
}

void Interpreter::visit(const program::IfStatement & is) 
{
  if (eval(is.condition).toBool())
    exec(is.body);
  else
  {
    if (is.elseClause)
      exec(is.elseClause);
  }
}

void Interpreter::visit(const program::PlacementStatement & placement)
{
  const int sp = mExecutionContext->stack.size;
  mExecutionContext->stack.push(eval(placement.object));
  for (const auto & arg : placement.arguments)
    mExecutionContext->stack.push(eval(arg));

  mExecutionContext->push(placement.constructor, sp);

  invoke(placement.constructor);

  mExecutionContext->pop();
}

void Interpreter::visit(const program::PushDataMember & ims)
{
  Object obj = mExecutionContext->callstack.top()->returnValue().toObject();
  auto impl = obj.implementation();
  impl->attributes.push_back(eval(ims.value));
}

void Interpreter::visit(const program::ReturnStatement & rs) 
{
  auto retval = rs.returnValue ? eval(rs.returnValue) : Value::Void;

  for (const auto & s : rs.destruction)
    exec(s);

  mExecutionContext->callstack.top()->setReturnValue(retval);
}

void Interpreter::visit(const program::PushGlobal & push)
{
  auto val = mExecutionContext->stack[push.global_index + mExecutionContext->callstack.top()->stackOffset()];
  Script s = mExecutionContext->callstack.top()->callee().script();
  s.implementation()->globals.push_back(val);
}

void Interpreter::visit(const program::PushValue & push) 
{
  auto val = eval(push.value);
  mExecutionContext->stack.push(val);
}

void Interpreter::visit(const program::PopDataMember & pop)
{
  const int object_offset = mExecutionContext->callstack.top()->stackOffset();
  Value object = mExecutionContext->stack[object_offset];
  auto impl = object.toObject().implementation();
  Value member = impl->attributes.back();
  mEngine->implementation()->destroy(member, pop.destructor);
  impl->attributes.pop_back();
}

void Interpreter::visit(const program::PopValue & pop) 
{
  if (pop.destroy)
  {
    mEngine->implementation()->destroy(mExecutionContext->stack.top(), pop.destructor);
  }

  mExecutionContext->stack.pop();
}

void Interpreter::visit(const program::WhileLoop & wl) 
{
  while (eval(wl.condition).toBool())
  {
    exec(wl.body);
    auto flags = mExecutionContext->flags();
    if (flags == FunctionCall::ReturnFlag)
      return;
    mExecutionContext->clearFlags();
    if (flags == FunctionCall::BreakFlag)
      break;
  }
}


Value Interpreter::visit(const program::ArrayExpression & array)
{
  Class array_class = mEngine->getClass(array.arrayType);
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(array_class.data());
  Array a{ std::make_shared<ArrayImpl>(array_data->data, mEngine) };
  auto aimpl = a.impl();
  aimpl->resize(array.elements.size());
  for (size_t i(0); i < array.elements.size(); ++i)
    aimpl->elements[i] = eval(array.elements.at(i));
  Value ret = Value::fromArray(a);
  mEngine->manage(ret);
  return ret;
}

Value Interpreter::visit(const program::BindExpression & bind)
{
  auto val = eval(bind.value);
  Context c = bind.context;
  c.addVar(bind.name, val);
  return val;
}

Value Interpreter::visit(const program::CaptureAccess & ca)
{
  Value value = eval(ca.lambda);
  LambdaObject lambda = value.toLambda();
  return lambda.captures().at(ca.offset);
}

Value Interpreter::visit(const program::CommaExpression & ce)
{
  eval(ce.lhs);
  return eval(ce.rhs);
}

Value Interpreter::visit(const program::ConditionalExpression & ce)
{
  if (eval(ce.cond).toBool())
    return eval(ce.onTrue);
  return eval(ce.onFalse);
}

Value Interpreter::visit(const program::ConstructorCall & call)
{
  const int sp = mExecutionContext->stack.size;

  Value object = mEngine->implementation()->buildValue(call.constructor.returnType().baseType());
  mEngine->manage(object);

  mExecutionContext->stack.push(object);
  for (const auto & arg : call.arguments)
    mExecutionContext->stack.push(eval(arg));

  mExecutionContext->push(call.constructor, sp);

  invoke(call.constructor);

  mExecutionContext->pop();

  return object;
}

Value Interpreter::visit(const program::Copy & copy)
{
  Value val = eval(copy.argument);
  Value ret = mEngine->copy(val);
  mEngine->manage(ret);
  return ret;
}

Value Interpreter::visit(const program::FetchGlobal & fetch)
{
  const Script & script = mExecutionContext->callstack.top()->callee().script();
  auto impl = script.implementation();
  return impl->globals[fetch.global_index];
}

Value Interpreter::visit(const program::FunctionCall & fc)
{
  const int sp = mExecutionContext->stack.size;
  mExecutionContext->stack.push(Value{});
  for (const auto & arg : fc.args)
    mExecutionContext->stack.push(eval(arg));

  mExecutionContext->push(fc.callee, sp);
  
  invoke(fc.callee);

  Value ret = mExecutionContext->pop();
  if (!(fc.callee.returnType().isReference() || fc.callee.returnType().isRefRef()))
    mEngine->manage(ret);
  return ret;
}

Value Interpreter::visit(const program::FunctionVariableCall & fvc)
{
  Value callee = eval(fvc.callee);
  Function f = callee.toFunction();

  const int sp = mExecutionContext->stack.size;
  mExecutionContext->stack.push(Value{});
  for (const auto & arg : fvc.arguments)
    mExecutionContext->stack.push(eval(arg));

  mExecutionContext->push(f, sp);

  invoke(f);

  Value ret = mExecutionContext->pop();
  if (!(f.returnType().isReference() || f.returnType().isRefRef()))
    mEngine->manage(ret);
  return ret;
}

Value Interpreter::visit(const program::FundamentalConversion & conv)
{
  Value src = eval(conv.argument);
  Value ret = fundamental_conversion(src, conv.dest_type.baseType().data(), mEngine);
  mEngine->manage(ret);
  return ret;
}

Value Interpreter::visit(const program::LambdaExpression & lexpr)
{
  Lambda closure_type = mEngine->getLambda(lexpr.closureType);
  auto limpl = std::make_shared<LambdaObjectImpl>(closure_type);

  for (const auto & cap : lexpr.captures)
    limpl->captures.push_back(eval(cap));

  auto ret = Value::fromLambda(LambdaObject{ limpl });
  mEngine->manage(ret);
  return ret;
}

Value Interpreter::visit(const program::Literal & l)
{
  return l.value;
}

Value Interpreter::visit(const program::LogicalAnd & la)
{
  Value cond = eval(la.lhs);
  if (!cond.toBool())
    return cond;

  return eval(la.rhs);
}

Value Interpreter::visit(const program::LogicalOr & lo)
{
  Value cond = eval(lo.lhs);
  if (cond.toBool())
    return cond;
  return eval(lo.rhs);
}

Value Interpreter::visit(const program::MemberAccess & ma)
{
  Object obj = eval(ma.object).toObject();
  return obj.getAttribute(ma.offset);
}

Value Interpreter::visit(const program::StackValue & sv)
{
  return mExecutionContext->stack[sv.stackIndex + mExecutionContext->callstack.top()->stackOffset()];
}

Value Interpreter::visit(const program::VariableAccess & va)
{
  return va.value;
}

Value Interpreter::visit(const program::VirtualCall & vc)
{
  Value object = eval(vc.object);

  const int sp = mExecutionContext->stack.size;
  mExecutionContext->stack.push(object);
  for (const auto & arg : vc.args)
    mExecutionContext->stack.push(eval(arg));

  Function callee = mEngine->getClass(object.type()).vtable().at(vc.vtableIndex);
  mExecutionContext->push(callee, sp);

  invoke(callee);

  Value ret = mExecutionContext->pop();
  if (!(callee.returnType().isReference() || callee.returnType().isRefRef()))
    mEngine->manage(ret);
  return ret;
}

} // namespace interpreter

} // namespace script
