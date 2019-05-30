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

  bool& get_bool() { return data.fundamentals.boolean; }
  void set_bool(bool bval) { data.fundamentals.boolean = bval; }
  char& get_char() { return data.fundamentals.character; }
  void set_char(char cval) { data.fundamentals.character = cval; }
  int& get_int() { return data.fundamentals.integer; }
  void set_int(int ival) { data.fundamentals.integer = ival; }
  float& get_float() { return data.fundamentals.realf; }
  void set_float(float fval) { data.fundamentals.realf = fval; }
  double& get_double() { return data.fundamentals.reald; }
  void set_double(double dval) { data.fundamentals.reald = dval; }

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
};

} // namespace script

#endif // LIBSCRIPT_VALUE_P_H
