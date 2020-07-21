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

struct LIBSCRIPT_API ValueImpl
{
  ValueImpl(Type t, Engine* e);
  ValueImpl(const ValueImpl&);
  ~ValueImpl();

  reference_counter_type ref;
  Type type;
  Engine* engine;
  void* data_ptr = nullptr;

  struct Fundamentals
  {
    bool boolean;
    char character;
    int integer;
    float realf;
    double reald;
  };

  union Data
  {
    Fundamentals fundamentals;
    String string;
    Object object;
    Array array;
    Function function;
    Lambda lambda;
    Enumerator enumerator;
    InitializerList initializer_list;
    char memory[LIBSCRIPT_BUILTIN_MEMBUF_SIZE];

    Data();
    ~Data();
  };
  Data data;

  enum Field {
    FundamentalsField,
    StringField,
    ObjectField,
    ArrayField,
    FunctionField,
    LambdaField,
    EnumeratorField,
    InitListField,
    MemoryField,
  };
  char which;

  bool& get_bool() { return *static_cast<bool*>(data_ptr); }
  void set_bool(bool bval) { data.fundamentals.boolean = bval; data_ptr = &(data.fundamentals.boolean); }
  char& get_char() { return *static_cast<char*>(data_ptr); }
  void set_char(char cval) { data.fundamentals.character = cval; data_ptr = &(data.fundamentals.character); }
  int& get_int() { return *static_cast<int*>(data_ptr); }
  void set_int(int ival) { data.fundamentals.integer = ival; data_ptr = &(data.fundamentals.integer); }
  float& get_float() { return *static_cast<float*>(data_ptr); }
  void set_float(float fval) { data.fundamentals.realf = fval; data_ptr = &(data.fundamentals.realf); }
  double& get_double() { return *static_cast<double*>(data_ptr); }
  void set_double(double dval) { data.fundamentals.reald = dval; data_ptr = &(data.fundamentals.reald); }

  String& get_string();
  void set_string(const String& sval);

  bool is_object() const;
  const Object& get_object() const;
  void init_object();
  void push_member(const Value& val);
  Value pop_member();
  Value get_member(size_t i) const;
  size_t member_count() const;

  bool is_array() const;
  const Array& get_array() const;
  void set_array(const Array& aval);

  bool is_function() const;
  const Function& get_function() const;
  void set_function(const Function& fval);
  bool is_lambda() const;
  const Lambda& get_lambda() const;
  void set_lambda(const Lambda& lval);
  const Enumerator& get_enumerator() const;
  void set_enumerator(const Enumerator& en);
  bool is_initializer_list() const;
  InitializerList get_initializer_list() const;
  void set_initializer_list(const InitializerList& il);

  void clear();

  void* acquire_memory();
  void release_memory();

  void* get_data() const;
};

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
  Enumerator value;

public:
  EnumeratorValue(const Enumerator& enm)
    : IValue(enm.enumeration().id(), enm.enumeration().engine()),
      value(enm)
  {

  }

  ~EnumeratorValue() = default;

  bool is_enumerator() const override { return true; }
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
