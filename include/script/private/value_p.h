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

namespace script
{

class Value;

#if defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
struct CharRef
{
  String* string;
  size_t pos;
};
#endif // defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)

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
#if defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
    CharRef charref;
#endif // defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
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
#if defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
    CharrefField,
#endif // defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
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

#if defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
  bool is_charref() const;
  CharRef& get_charref();
  void set_charref(const CharRef& cr);
#endif // defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)

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

} // namespace script

#endif // LIBSCRIPT_VALUE_P_H
