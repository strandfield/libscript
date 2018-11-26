// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_EXECUTION_CONTEXT_H
#define LIBSCRIPT_EXECUTION_CONTEXT_H

#include "script/function.h"
#include "script/types.h"
#include "script/value.h"

namespace script
{

namespace interpreter
{

class Interpreter;

struct Stack
{
  Stack();
  Stack(int c);
  ~Stack();

  typedef Value* iterator;
  typedef const Value* const_iterator;

  int size;
  int capacity;
  Value *data;

  void push(const Value & val);
  Value & top();
  const Value & top() const;
  Value pop();

  Value & operator[](int index);

public:
  Stack(const Stack &) = delete;
  Stack & operator=(const Stack &) = delete;
};

class LIBSCRIPT_API StackView
{
public:
  StackView(Stack *s, int begin, int end);
  StackView(const StackView &) = default;
  ~StackView();

  int size() const;
  Value at(int index) const;

  Stack::iterator begin() const;
  Stack::iterator end() const;

  StackView & operator=(const StackView & other) = default;

private:
  Stack *mStack;
  int mBegin;
  int mEnd;
};

class ExecutionContext;

class LIBSCRIPT_API FunctionCall
{
public:
  FunctionCall();

  FunctionCall * caller() const;
  Function callee() const;

  void setReturnValue(const Value & val);
  Value & returnValue();
  Value & thisObject() const;
  StackView args() const;
  Value arg(int index) const;

  void initObject();
  void push(const Value & val);
  Value pop();
  void destroyObject();

  ExecutionContext * executionContext() const;
  Engine * engine() const;

  inline int stackOffset() const { return mStackIndex; }

  void setBreakFlag();
  void setContinueFlag();
  void clearFlags();

  // this and the flags data member could be moved to the ExecutionContext class
  enum Flag {
    NoFlags = 0,
    BreakFlag = 1,
    ContinueFlag = 2,
    ReturnFlag = 4
  };

private:
  friend class Callstack;
  friend class ExecutionContext;
private:
  Function mCallee;
  int mIndex; // index in the callstack
  int mStackIndex; // index of return value in the callstack
  int mArgc;
  int flags;
  ExecutionContext *ec;
};

class Callstack
{
public:
  Callstack(int capacity);
  Callstack(const Callstack &) = delete;
  ~Callstack() = default;

  int capacity() const;
  int size();

  FunctionCall * push();
  FunctionCall * top();
  const FunctionCall * top() const;
  void pop();

  Callstack & operator=(const Callstack &) = delete;
  FunctionCall * operator[](int index);

private:
  std::vector<FunctionCall> mData;
  int mSize;
};


class ExecutionContext
{
public:
  ExecutionContext(Engine *e, int stackSize, int callStackSize);
  ~ExecutionContext();

  bool push(const Function & f, Value *begin, Value *end);
  bool push(const Function & f, int sp);
  Value pop();

  int flags() const;
  void clearFlags();

  Engine *engine;
  Callstack callstack;
  Stack stack;
  std::vector<Value> initializer_list_buffer;
  Function initializer_list_owner;
  std::vector<Value> garbage_collector;
};

} // namespace interpreter

typedef interpreter::FunctionCall FunctionCall;

} // namespace script

#endif // LIBSCRIPT_EXECUTION_CONTEXT_H
