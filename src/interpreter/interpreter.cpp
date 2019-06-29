// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/interpreter/interpreter.h"

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/context.h"
#include "script/initializerlist.h"
#include "script/object.h"
#include "script/script.h"

#include "script/private/array_p.h"
#include "script/private/function_p.h"
#include "script/private/lambda_p.h"
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

  mExecutionContext->stack.push(Value::Void);

  if (obj)
    mExecutionContext->stack.push(*obj);
  
  const Prototype & proto = f.prototype();
  const int argc = std::distance(begin, end);

  assert(argc + (obj ? 1 : 0) == f.prototype().count());

  try
  {
    const int offset = (obj != nullptr ? 1 : 0);
    for (int i(0); i < argc; ++i)
    {
      Value arg = e->cast(begin[i], proto.at(i + offset));
      if (!proto.at(i).isReference())
        e->manage(arg);
      mExecutionContext->stack.push(arg);
    }
  }
  catch (const std::runtime_error &)
  {
    // pop the arguments
    while (mExecutionContext->stack.size > sp + 1)
      mExecutionContext->stack.pop();

    mExecutionContext->stack.pop();

    throw;
  }

  mExecutionContext->push(f, sp);

  invoke(f);

  return mExecutionContext->pop();
}

Value Interpreter::invoke(const Function & f, const Value *obj, const Value *begin, const Value *end)
{
  mExecutionContext->push(f, obj, begin, end);
  invoke(f);
  return mExecutionContext->pop();
}

Value Interpreter::eval(const std::shared_ptr<program::Expression> & expr)
{
  const size_t gcs = mExecutionContext->garbage_collector.size();
  const size_t ilistbuffersize = mExecutionContext->initializer_list_buffer.size();
  
  Value ret = inner_eval(expr);

  // Destroy temporaries
  while (mExecutionContext->garbage_collector.size() > gcs)
  {
    Value & v = mExecutionContext->garbage_collector.back();
    if(v.impl()->ref == 1)
      mEngine->destroy(v);
    mExecutionContext->garbage_collector.pop_back();
  }

  // Destroy initializer lists
  while (mExecutionContext->initializer_list_buffer.size() > ilistbuffersize)
  {
    Value & v = mExecutionContext->initializer_list_buffer.back();
    /// TODO: should we do something if refcount is not 1 ?
    if (v.impl()->ref == 1)
      mEngine->destroy(v);
    mExecutionContext->initializer_list_buffer.pop_back();
  }

  return ret;
}

bool Interpreter::evalCondition(const std::shared_ptr<program::Expression> & expr)
{
  Value v = eval(expr);
  const bool ret = v.toBool();
  if (v.impl()->ref == 1)
    mEngine->destroy(v);
  return ret;
}

void Interpreter::evalForSideEffects(const std::shared_ptr<program::Expression> & expr)
{
  Value v = eval(expr);
  if (v.impl()->ref == 1)
    mEngine->destroy(v);
}

Value Interpreter::inner_eval(const std::shared_ptr<program::Expression> & expr)
{
  return manage(expr->accept(*this));
}

Value Interpreter::manage(const Value & val)
{
  mExecutionContext->garbage_collector.push_back(val);
  return val;
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
  auto impl = f.impl();
  if (f.isNative()) {
    interpreter::FunctionCall *fcall = mExecutionContext->callstack.top();
    fcall->setReturnValue(impl->implementation.callback(fcall));
  } else {
    exec(impl->implementation.program);
  }

  /// TODO: maybe remove this call to the GC
  mEngine->garbageCollect();
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
  Value & memplace = *mExecutionContext->callstack.top()->args().begin();
  memplace.impl()->init_object();
}

void Interpreter::visit(const program::ExpressionStatement & es) 
{
  Value v = eval(es.expr);
  if (v.impl()->ref == 1)
    mEngine->destroy(v);
}

void Interpreter::visit(const program::ForLoop & fl) 
{
  exec(fl.init);
  while (evalCondition(fl.cond))
  {
    exec(fl.body);

    auto flags = mExecutionContext->flags();
    if (flags == FunctionCall::ReturnFlag)
      return;
    mExecutionContext->clearFlags();
    if (flags == FunctionCall::BreakFlag)
      return; // the break statement should have destroyed the variable in the init-scope.

    evalForSideEffects(fl.loop);
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
  mExecutionContext->stack.push(Value::Void);
  mExecutionContext->stack.push(eval(placement.object));
  for (const auto & arg : placement.arguments)
    mExecutionContext->stack.push(eval(arg));

  mExecutionContext->push(placement.constructor, sp);

  invoke(placement.constructor);

  mExecutionContext->pop();
}

void Interpreter::visit(const program::PushDataMember & ims)
{
  Value object = mExecutionContext->callstack.top()->arg(0);
  Value member = eval(ims.value);
  object.impl()->push_member(member);
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
  Script s = mExecutionContext->engine->implementation()->scripts.at(push.script_index);
  s.impl()->globals.push_back(val);
}

void Interpreter::visit(const program::PushValue & push) 
{
  auto val = eval(push.value);
  mExecutionContext->stack.push(val);
}

void Interpreter::visit(const program::PopDataMember & pop)
{
  Value object = mExecutionContext->callstack.top()->arg(0);
  mEngine->implementation()->destroy(object.impl()->pop_member(), pop.destructor);
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
  while (evalCondition(wl.condition))
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
    aimpl->elements[i] = inner_eval(array.elements.at(i));
  return Value::fromArray(a);
}

Value Interpreter::visit(const program::BindExpression & bind)
{
  auto val = inner_eval(bind.value);
  Context c = bind.context;
  c.addVar(bind.name, val);
  return val;
}

Value Interpreter::visit(const program::CaptureAccess & ca)
{
  Value value = inner_eval(ca.lambda);
  Lambda lambda = value.toLambda();
  return lambda.captures().at(ca.offset);
}

Value Interpreter::visit(const program::CommaExpression & ce)
{
  inner_eval(ce.lhs);
  return inner_eval(ce.rhs);
}

Value Interpreter::visit(const program::ConditionalExpression & ce)
{
  if (inner_eval(ce.cond).toBool())
    return inner_eval(ce.onTrue);
  return inner_eval(ce.onFalse);
}

Value Interpreter::visit(const program::ConstructorCall & call)
{
  const int sp = mExecutionContext->stack.size;

  Value object = mEngine->allocate(call.allocate->object_type);

  mExecutionContext->stack.push(Value::Void);
  mExecutionContext->stack.push(object);
  for (const auto & arg : call.arguments)
    mExecutionContext->stack.push(inner_eval(arg));

  mExecutionContext->push(call.constructor, sp);

  invoke(call.constructor);

  mExecutionContext->pop();

  return object;
}

Value Interpreter::visit(const program::Copy & copy)
{
  Value val = inner_eval(copy.argument);
  Value ret = mEngine->copy(val);
  return ret;
}

Value Interpreter::visit(const program::FetchGlobal & fetch)
{
  const Script & script = mExecutionContext->engine->implementation()->scripts.at(fetch.script_index);
  auto impl = script.impl();
  return impl->globals[fetch.global_index];
}

Value Interpreter::visit(const program::FunctionCall & fc)
{
  const int sp = mExecutionContext->stack.size;
  mExecutionContext->stack.push(Value::Void);
  for (const auto & arg : fc.args)
    mExecutionContext->stack.push(inner_eval(arg));

  mExecutionContext->push(fc.callee, sp);
  
  invoke(fc.callee);

  Value ret = mExecutionContext->pop();
  return ret;
}

Value Interpreter::visit(const program::FunctionVariableCall & fvc)
{
  Value callee = inner_eval(fvc.callee);
  Function f = callee.toFunction();

  const int sp = mExecutionContext->stack.size;
  mExecutionContext->stack.push(Value::Void);
  for (const auto & arg : fvc.arguments)
    mExecutionContext->stack.push(inner_eval(arg));

  mExecutionContext->push(f, sp);

  invoke(f);

  Value ret = mExecutionContext->pop();
  return ret;
}

Value Interpreter::visit(const program::FundamentalConversion & conv)
{
  Value src = inner_eval(conv.argument);
  Value ret = fundamental_conversion(src, conv.dest_type.baseType().data(), mEngine);
  return ret;
}

Value Interpreter::visit(const program::InitializerList & il)
{
  const size_t old_size = mExecutionContext->initializer_list_buffer.size();

  for (const auto & e : il.elements)
  {
    Value val = inner_eval(e);
    mExecutionContext->initializer_list_buffer.push_back(val);
  }

  const size_t new_size = mExecutionContext->initializer_list_buffer.size();

  Value ret = mEngine->construct(il.initializer_list_type, {});

  Value* begin = mExecutionContext->initializer_list_buffer.data() + old_size;
  Value* end = mExecutionContext->initializer_list_buffer.data() + new_size;
  ret.impl()->set_initializer_list(InitializerList{ begin, end });
  return ret;
}

Value Interpreter::visit(const program::LambdaExpression & lexpr)
{
  ClosureType closure_type = mEngine->getLambda(lexpr.closureType);
  auto limpl = std::make_shared<LambdaImpl>(closure_type);

  for (const auto & cap : lexpr.captures)
    limpl->captures.push_back(inner_eval(cap));

  auto ret = Value::fromLambda(Lambda{ limpl });
  return ret;
}

Value Interpreter::visit(const program::Literal & l)
{
  return l.value;
}

Value Interpreter::visit(const program::LogicalAnd & la)
{
  Value cond = inner_eval(la.lhs);
  if (!cond.toBool())
    return cond;

  return inner_eval(la.rhs);
}

Value Interpreter::visit(const program::LogicalOr & lo)
{
  Value cond = inner_eval(lo.lhs);
  if (cond.toBool())
    return cond;
  return inner_eval(lo.rhs);
}

Value Interpreter::visit(const program::MemberAccess & ma)
{
  Value object = inner_eval(ma.object);
  return object.impl()->get_member(ma.offset);
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
  Value object = inner_eval(vc.object);

  const int sp = mExecutionContext->stack.size;
  mExecutionContext->stack.push(Value{});
  mExecutionContext->stack.push(object);
  for (const auto & arg : vc.args)
    mExecutionContext->stack.push(inner_eval(arg));

  Function callee = mEngine->getClass(object.type()).vtable().at(vc.vtableIndex);
  mExecutionContext->push(callee, sp);

  invoke(callee);

  Value ret = mExecutionContext->pop();
  return ret;
}

} // namespace interpreter

} // namespace script
