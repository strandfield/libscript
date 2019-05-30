// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_EXECUTION_CONTEXT_H
#define LIBSCRIPT_EXECUTION_CONTEXT_H

#include "script/function.h"
#include "script/thisobject.h"
#include "script/types.h"

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
  inline const Function & callee() const { return mCallee; }

  void setReturnValue(const Value & val);
  Value & returnValue();
  StackView args() const;
  Value arg(int index) const;
  inline int argc() const { return callee().prototype().count(); }

  ThisObject thisObject() const;

  /// TODO: remove these
  void initObject() { thisObject().init(); }
  void push(const Value& val) { thisObject().push(val); }
  Value pop() { return thisObject().pop(); }
  void destroyObject() { thisObject().destroy(); }

  ExecutionContext * executionContext() const;
  Engine * engine() const;

  inline int stackOffset() const { return mStackIndex; }
  int depth() const;

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
  int mStackIndex; // index of return value in the callstack
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

  FunctionCall * push(const Function & f, int stackOffset);
  FunctionCall * top();
  const FunctionCall * top() const;
  void pop();

  const FunctionCall* begin() const;
  const FunctionCall* end() const;

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

  void push(const Function & f, const Value *obj, const Value *begin, const Value *end);
  bool push(const Function & f, int sp);
  Value pop();

  int flags() const;
  void clearFlags();

  Engine *engine;
  Callstack callstack;
  Stack stack;
  std::vector<Value> initializer_list_buffer;
  std::vector<Value> garbage_collector;
};

} // namespace interpreter

typedef interpreter::FunctionCall FunctionCall;

} // namespace script

#endif // LIBSCRIPT_EXECUTION_CONTEXT_H
