// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_P_H
#define LIBSCRIPT_VALUE_P_H

#include "script/array.h"
#include "script/enumerator.h"
#include "script/function.h"
#include "script/initializerlist.h"
#include "script/lambda.h"
#include "script/object.h"
#include "script/string.h"
#include "script/value-interface.h"

#include <vector>

namespace script
{

class Value;

class VoidValue : public IValue
{
public:
  VoidValue()
    : IValue(script::Type::Void, nullptr)
  {

  }

  ~VoidValue() = default;

  bool is_void() const override { return true; }
  void* ptr() override { return nullptr; }
};

class FunctionValue : public IValue
{
public:
  Function function;

public:
  FunctionValue(const Function& f, const Type& ft)
    : IValue(ft, f.engine()),
      function(f)
  {

  }

  ~FunctionValue() = default;

  bool is_function() const override { return true; }
  void* ptr() override { return &function; }
};

class LambdaValue : public IValue
{
public:
  Lambda lambda;

public:
  LambdaValue(const Lambda& l)
    : IValue(l.closureType().id(), l.engine()), 
      lambda(l)
  {

  }

  ~LambdaValue() = default;

  bool is_lambda() const override { return true; }
  void* ptr() override { return &lambda; }
};

class ArrayValue : public IValue
{
public:
  Array array;

public:
  ArrayValue(const Array& a)
    : IValue(a.typeId(), a.engine()),
      array(a)
  {

  }

  ~ArrayValue() = default;

  bool is_array() const override { return true; }
  void* ptr() override { return &array; }
};

class InitializerListValue : public IValue
{
public:
  InitializerList initlist;

public:

  InitializerListValue(script::Engine* e, script::Type t, const InitializerList& ilist)
    : IValue(t, e), 
      initlist(ilist)
  {

  }

  ~InitializerListValue() = default;

  bool is_initializer_list() const override { return true; }
  void* ptr() override { return &initlist; }
};

class EnumeratorValue : public IValue
{
public:
  using value_type = int;
  value_type value;

public:
  EnumeratorValue(const Enumerator& enm)
    : IValue(enm.enumeration().id(), enm.enumeration().engine()),
      value(enm.value())
  {

  }

  ~EnumeratorValue() = default;

  bool is_enum() const override { return true; }
  int get_enum_value() const override { return value; }
  void* ptr() override { return &value; }
};

class ScriptValue : public IValue
{
public:
  std::vector<Value> members;

public:

  ScriptValue(script::Engine* e, script::Type t)
    : IValue(t, e)
  {

  }

  void* ptr() override { return nullptr; }
  size_t size() const override { return members.size(); }
  void push(const Value& val) override { members.push_back(val); }
  Value pop() override { Value back = members.back();  members.pop_back(); return back; }
  Value& at(size_t index) override { return members.at(index); }
};

} // namespace script

#endif // LIBSCRIPT_VALUE_P_H
