// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/interpreter/executioncontext.h"

#include "script/value.h"

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
  return this->data[--this->size];
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
  : mIndex(0)
  , mStackIndex(0)
  , mThisOffset(1)
  , mArgc(0)
  , flags(0)
  , ec(nullptr)
{

}

FunctionCall * FunctionCall::caller() const
{
  if (mIndex == 0)
    return nullptr;

  return this->ec->callstack[mIndex - 1];
}

Function FunctionCall::callee() const
{
  return mCallee;
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

Value & FunctionCall::thisObject() const
{
  return this->ec->stack[this->mStackIndex + this->mThisOffset];
}

Value FunctionCall::arg(int index) const
{
  return this->ec->stack[this->mStackIndex + index + 1];
}

StackView FunctionCall::args() const
{
  return StackView{ &this->ec->stack, this->mStackIndex + 1, this->mStackIndex + 1 + this->mArgc };
}

ExecutionContext * FunctionCall::executionContext() const
{
  return this->ec;
}

Engine * FunctionCall::engine() const
{
  return this->ec->engine;
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

FunctionCall * Callstack::push()
{
  if (size() == capacity())
    throw std::runtime_error{ "Callstack overflow" };

  FunctionCall *ret = std::addressof(mData[mSize++]);
  ret->mIndex = mSize - 1;
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


FunctionCall * Callstack::operator[](int index)
{
  return std::addressof(mData[index]);
}



ExecutionContext::ExecutionContext(Engine *e, int stackSize, int callStackSize)
  : engine(e)
  , stack(stackSize)
  , callstack(callStackSize)
{

}

ExecutionContext::~ExecutionContext()
{
  engine = nullptr;
}

bool ExecutionContext::push(const Function & f, Value *begin, Value *end)
{
  try {
    FunctionCall *fc = this->callstack.push();
    fc->mCallee = f;
    fc->mArgc = std::distance(begin, end);
    fc->mStackIndex = this->stack.size;
    fc->mThisOffset = f.isConstructor() || f.isDestructor() ? 0 : 1;
    this->stack.size += 1;
    for (auto it = begin; it != end; ++it)
    {
      this->stack[this->stack.size] = *it;
      this->stack.size += 1;
    }
    fc->flags = FunctionCall::NoFlags;
    fc->ec = this;
  }
  catch (...)
  {
    return false;
  }

  return true;
}

bool ExecutionContext::push(const Function & f, int sp)
{
  try {
    FunctionCall *fc = this->callstack.push();
    fc->mCallee = f;
    fc->mArgc = this->stack.size - sp - 1;
    fc->mStackIndex = sp;
    fc->mThisOffset = f.isConstructor() || f.isDestructor() ? 0 : 1;
    fc->flags = FunctionCall::NoFlags;
    fc->ec = this;
  }
  catch (...)
  {
    return false;
  }

  return true;
}

Value ExecutionContext::pop()
{
  FunctionCall *fc = this->callstack.top();
  this->stack.size -= (fc->mArgc);
  this->callstack.pop();
  /// TODO : generate implicit return statement in functions instead of this
  Value ret = this->stack.pop();
  if (ret.isNull())
    return Value::Void;
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

