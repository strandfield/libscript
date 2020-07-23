// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/interpreter/executioncontext.h"

#include "script/engine.h"
#include "script/value.h"
#include "script/private/value_p.h"

#include <stdexcept>

namespace script
{

namespace interpreter
{

Stack::Stack() : size(0), capacity(0), data(0) { }

Stack::Stack(int c) : size(0), capacity(c)
{
  this->data = new Value[c];
}

Stack::~Stack()
{
  if (this->data)
    delete[] this->data;
}

void Stack::push(const Value & val)
{
  this->data[this->size++] = val;
}

Value & Stack::top()
{
  return this->data[this->size - 1];
}

const Value & Stack::top() const
{
  return this->data[this->size - 1];
}

Value Stack::pop()
{
  Value ret = this->data[--this->size];
  this->data[this->size] = Value{};
  return ret;
}

Value & Stack::operator[](int index)
{
  return this->data[index];
}

StackView::StackView(Stack *s, int begin, int end)
  : mStack(s)
  , mBegin(begin)
  , mEnd(end)
{

}

StackView::~StackView()
{
  mStack = nullptr;
  mBegin = 0;
  mEnd = 0;
}

int StackView::size() const
{
  return mEnd - mBegin;
}

Value StackView::at(int index) const
{
  return mStack->data[mBegin + index];
}

Stack::iterator StackView::begin() const
{
  return mStack->data + mBegin;
}

Stack::iterator StackView::end() const
{
  return mStack->data + mEnd;
}


FunctionCall::FunctionCall()
  : mStackIndex(0)
  , flags(0)
  , ec(nullptr)
{

}

FunctionCall * FunctionCall::caller() const
{
  const int d = depth();

  if (d == 0)
    return nullptr;

  return this->ec->callstack[d - 1];
}

void FunctionCall::setReturnValue(const Value & val)
{
  this->ec->stack[this->mStackIndex] = val;
  this->flags = ReturnFlag;
}

Value & FunctionCall::returnValue()
{
  return this->ec->stack[this->mStackIndex];
}

ThisObject FunctionCall::thisObject() const
{
  return ThisObject(this->ec->stack[this->mStackIndex + 1], this->engine());
}

Value FunctionCall::arg(int index) const
{
  return this->ec->stack[this->mStackIndex + index + 1];
}

StackView FunctionCall::args() const
{
  return StackView{ &this->ec->stack, stackOffset() + 1, stackOffset() + 1 + argc() };
}

ExecutionContext * FunctionCall::executionContext() const
{
  return this->ec;
}

Engine * FunctionCall::engine() const
{
  return this->ec->engine;
}

TypeSystem* FunctionCall::typeSystem() const
{
  return this->ec->engine->typeSystem();
}

int FunctionCall::depth() const
{
  return std::distance(this->ec->callstack.begin(), this);
}

void FunctionCall::setBreakFlag()
{
  this->flags = BreakFlag;
}

void FunctionCall::setContinueFlag()
{
  this->flags = ContinueFlag;
}

void FunctionCall::clearFlags()
{
  this->flags = 0;
}



Callstack::Callstack(int capacity)
  : mSize(0)
{
  mData.resize(capacity);
}

int Callstack::capacity() const
{
  return mData.capacity();
}

int Callstack::size()
{
  return mSize;
}

FunctionCall * Callstack::push(const Function & f, int stackOffset)
{
  if (size() == capacity())
    throw std::runtime_error{ "Callstack overflow" };

  FunctionCall *ret = std::addressof(mData[mSize++]);
  ret->mCallee = f;
  ret->mStackIndex = stackOffset;
  ret->flags = FunctionCall::NoFlags;
  return ret;
}

FunctionCall * Callstack::top() 
{
  return std::addressof(mData[mSize-1]);
}

const FunctionCall * Callstack::top() const
{
  return std::addressof(mData.at(mSize-1));
}

void Callstack::pop()
{
  assert(mSize > 0);
  --mSize;
}

const FunctionCall* Callstack::begin() const
{
  return std::addressof(mData[0]);
}

const FunctionCall* Callstack::end() const
{
  return std::addressof(mData[mSize]);
}

FunctionCall * Callstack::operator[](int index)
{
  return std::addressof(mData[index]);
}



ExecutionContext::ExecutionContext(Engine *e, int stackSize, int callStackSize)
  : engine(e)
  , stack(stackSize)
  , callstack(callStackSize)
{
  /// TODO: size must never exceed initial reserved amount, check for that
  // (otherwise some instances will become invalid)
  initializer_list_buffer.reserve(256);
}

ExecutionContext::~ExecutionContext()
{
  engine = nullptr;
}

void ExecutionContext::push(const Function & f, const Value *obj, const Value *begin, const Value *end)
{
  FunctionCall *fc = this->callstack.push(f, this->stack.size);
  this->stack[this->stack.size++] = Value::Void;
  if (obj != nullptr)
    this->stack[this->stack.size++] = *obj;
  for (auto it = begin; it != end; ++it)
    this->stack[this->stack.size++] = *it;
  fc->ec = this;
}

void ExecutionContext::push(const Function & f, int sp)
{
  FunctionCall* fc = this->callstack.push(f, sp);
  fc->ec = this;
}

Value ExecutionContext::pop()
{
  FunctionCall *fc = this->callstack.top();
  for (int i = 0; i < fc->argc(); ++i)
    this->stack.pop();
  this->callstack.pop();

  Value ret = this->stack.pop();
  return ret;
}

int ExecutionContext::flags() const
{
  return this->callstack.top()->flags;
}

void ExecutionContext::clearFlags()
{
  this->callstack.top()->clearFlags();
}

} // namespace interpreter

} // namespace script

