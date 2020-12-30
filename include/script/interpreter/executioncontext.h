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

class TypeSystem;

namespace program
{
struct Breakpoint;
} // namespace program

namespace interpreter
{

class Interpreter;

struct Stack
{
  Stack();
  Stack(size_t c);
  ~Stack();

  typedef Value* iterator;
  typedef const Value* const_iterator;

  size_t size;
  size_t capacity;
  Value *data;

  void push(const Value& val);
  Value& top();
  const Value& top() const;
  Value pop();

  Value& operator[](size_t index);

public:
  Stack(const Stack &) = delete;
  Stack & operator=(const Stack &) = delete;
};

class LIBSCRIPT_API StackView
{
public:
  StackView(Stack *s, size_t begin, size_t end);
  StackView(const StackView &) = default;
  ~StackView();

  size_t size() const;
  Value at(size_t index) const;

  Stack::iterator begin() const;
  Stack::iterator end() const;

  StackView & operator=(const StackView & other) = default;

private:
  Stack *mStack;
  size_t mBegin;
  size_t mEnd;
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
  inline size_t argc() const { return callee().prototype().count(); }

  ThisObject thisObject() const;

  ExecutionContext * executionContext() const;
  Engine * engine() const;
  TypeSystem* typeSystem() const;

  inline size_t stackOffset() const { return mStackIndex; }
  size_t depth() const;

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
  size_t mStackIndex; // index of return value in the callstack
  int flags;
  ExecutionContext *ec;
public:
  const program::Breakpoint* last_breakpoint = nullptr;
};

class LIBSCRIPT_API Callstack
{
public:
  Callstack(size_t capacity);
  Callstack(const Callstack &) = delete;
  ~Callstack() = default;

  size_t capacity() const;
  size_t size();

  FunctionCall* push(const Function& f, size_t stackOffset);
  FunctionCall* top();
  const FunctionCall* top() const;
  void pop();

  const FunctionCall* begin() const;
  const FunctionCall* end() const;

  Callstack& operator=(const Callstack&) = delete;
  FunctionCall* operator[](size_t index);

private:
  std::vector<FunctionCall> mData;
  size_t mSize;
};


class ExecutionContext
{
public:
  ExecutionContext(Engine *e, size_t stackSize, size_t callStackSize);
  ~ExecutionContext();

  void push(const Function & f, const Value *obj, const Value *begin, const Value *end);
  void push(const Function & f, size_t sp);
  Value pop();

  int flags() const;
  void clearFlags();

  Engine* engine;
  Callstack callstack;
  Stack stack;
  std::vector<Value> initializer_list_buffer;
  std::vector<Value> garbage_collector;
};

} // namespace interpreter

typedef interpreter::FunctionCall FunctionCall;

} // namespace script

#endif // LIBSCRIPT_EXECUTION_CONTEXT_H
