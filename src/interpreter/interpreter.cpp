// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/interpreter/interpreter.h"

#include "script/interpreter/debug-handler.h"

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/context.h"
#include "script/initializerlist.h"
#include "script/object.h"
#include "script/script.h"
#include "script/typesystem.h"

#include "script/private/array_p.h"
#include "script/private/function_p.h"
#include "script/private/lambda_p.h"
#include "script/private/script_p.h"
#include "script/private/value_p.h"

namespace script
{

namespace interpreter
{

struct Invoker
{
  ExecutionContext& context;
  size_t sp;
  bool preparing;

public:
  Invoker(ExecutionContext& ec, const Function& f, const Value* obj, const Value* begin, const Value* end)
    : context(ec), sp(context.stack.size), preparing(false)
  {
    context.push(f, obj, begin, end);
  }

  Invoker(ExecutionContext& ec)
    : context(ec), sp(context.stack.size), preparing(true)
  {

  }

  void push(const Function& f)
  {
    context.push(f, sp);
    preparing = false;
  }

  ~Invoker()
  {
    if (std::uncaught_exceptions())
    {
      while (context.stack.size > sp)
        context.stack.pop();

      if(!preparing)
        context.callstack.pop();
    }
  }
};

class DefaultDebugHandler : public DebugHandler
{
public:
  void interrupt(FunctionCall&, program::Breakpoint&) override { }
};

Interpreter::Interpreter(std::shared_ptr<ExecutionContext> ec, Engine *e)
  : mEngine(e)
  , mExecutionContext(ec)
  , mDebugHandler(std::make_shared<DefaultDebugHandler>())
{

}

Interpreter::~Interpreter()
{
  // Interesting note: we cannot make this destructor = default
  // because in the header, the InterpreterImpl type is not defined 
  // and thus its destructor is not available (causes a compile error).
}

Value Interpreter::invoke(const Function & f, const Value *obj, const Value *begin, const Value *end)
{
  Invoker invoker{ *mExecutionContext, f, obj, begin, end };
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

void Interpreter::setDebugHandler(std::shared_ptr<DebugHandler> h)
{
  if (h)
    mDebugHandler = h;
  else
    mDebugHandler = std::make_shared<DefaultDebugHandler>();
}


void Interpreter::invoke(const Function & f)
{
  auto impl = f.impl();

  if (f.isNative()) 
  {
    interpreter::FunctionCall* fcall = mExecutionContext->callstack.top();
    fcall->setReturnValue(f.impl()->invoke(fcall));
  } 
  else 
  {
    exec(f.program());
  }
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
  memplace = Value(new ScriptValue(mExecutionContext->engine, cos.objectType));
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

void Interpreter::visit(const program::ConstructionStatement & construction)
{
  Invoker invoker{ *mExecutionContext };

  mExecutionContext->stack.push(Value::Void);
  mExecutionContext->stack.push(Value::Void);
  for (const auto & arg : construction.arguments)
    mExecutionContext->stack.push(eval(arg));

  invoker.push(construction.constructor);

  invoke(construction.constructor);

  Value object = mExecutionContext->pop();
  object.impl()->type = construction.object_type;

  mExecutionContext->stack[mExecutionContext->callstack.top()->stackOffset() + 1] = object;
}

void Interpreter::visit(const program::PushDataMember & ims)
{
  Value object = mExecutionContext->callstack.top()->arg(0);
  Value member = eval(ims.value);
  object.impl()->push(member);
}

void Interpreter::visit(const program::ReturnStatement & rs) 
{
  auto retval = rs.returnValue ? eval(rs.returnValue) : Value::Void;

  for (const auto & s : rs.destruction)
    exec(s);

  mExecutionContext->callstack.top()->setReturnValue(retval);
}

void Interpreter::visit(const program::CppReturnStatement& rs)
{
  script::FunctionCall* c = mExecutionContext->callstack.top();
  script::Value r = rs.native_fun(c);
  c->setReturnValue(r);
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

void Interpreter::visit(const program::PushStaticValue& push)
{
  Script s = mExecutionContext->engine->implementation()->scripts.at(push.script_index);
  
  Value& val = s.impl()->static_variables[push.static_index];

  if (val.isNull())
    val = eval(push.expr);

  mExecutionContext->stack.push(val);
}

void Interpreter::visit(const program::PopDataMember & pop)
{
  Value object = mExecutionContext->callstack.top()->arg(0);
  mEngine->implementation()->destroy(object.impl()->pop(), pop.destructor);
}

void Interpreter::visit(const program::PopValue & pop) 
{
  if (pop.destroy)
  {
    mEngine->implementation()->destroy(mExecutionContext->stack.top(), pop.destructor);
  }

  (void) mExecutionContext->stack.pop();
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

void Interpreter::visit(const program::Breakpoint& bp)
{
  mExecutionContext->callstack.top()->last_breakpoint = &bp;
  mDebugHandler->interrupt(*mExecutionContext->callstack.top(), const_cast<program::Breakpoint&>(bp));
}


Value Interpreter::visit(const program::ArrayExpression & array)
{
  Class array_class = mEngine->typeSystem()->getClass(array.arrayType);
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(array_class.data());
  Array a{ std::make_shared<ArrayImpl>(array_data->data, mEngine) };
  auto aimpl = a.impl();
  aimpl->resize(static_cast<int>(array.elements.size()));
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
  (void) inner_eval(ce.lhs);
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
  Invoker invoker{ *mExecutionContext };

  mExecutionContext->stack.push(Value::Void); // ret
  mExecutionContext->stack.push(Value::Void); // this
  for (const auto & arg : call.arguments)
    mExecutionContext->stack.push(inner_eval(arg));

  invoker.push(call.constructor);

  invoke(call.constructor);

  return mExecutionContext->pop();
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
  Invoker invoker{ *mExecutionContext };

  mExecutionContext->stack.push(Value::Void);
  for (const auto & arg : fc.args)
    mExecutionContext->stack.push(inner_eval(arg));
  
  invoker.push(fc.callee);

  invoke(fc.callee);

  return mExecutionContext->pop();
}

Value Interpreter::visit(const program::FunctionVariableCall & fvc)
{
  Value callee = inner_eval(fvc.callee);
  Function f = callee.toFunction();

  Invoker invoker{ *mExecutionContext };

  mExecutionContext->stack.push(Value::Void);
  for (const auto & arg : fvc.arguments)
    mExecutionContext->stack.push(inner_eval(arg));

  invoker.push(f);

  invoke(f);

  return mExecutionContext->pop();
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

  Value* begin = mExecutionContext->initializer_list_buffer.data() + old_size;
  Value* end = mExecutionContext->initializer_list_buffer.data() + new_size;
  return Value(new InitializerListValue(mExecutionContext->engine, il.initializer_list_type, InitializerList{ begin, end }));
}

Value Interpreter::visit(const program::LambdaExpression & lexpr)
{
  ClosureType closure_type = mEngine->typeSystem()->getLambda(lexpr.closureType);
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
  return object.impl()->at(ma.offset);
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
  Function callee = mEngine->typeSystem()->getClass(object.type()).vtable().at(vc.vtableIndex);

  Invoker invoker{ *mExecutionContext };

  mExecutionContext->stack.push(Value{});
  mExecutionContext->stack.push(object);
  for (const auto & arg : vc.args)
    mExecutionContext->stack.push(inner_eval(arg));

  invoker.push(callee);

  invoke(callee);

  return mExecutionContext->pop();
}

} // namespace interpreter

} // namespace script
