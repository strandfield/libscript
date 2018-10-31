// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_P_H
#define LIBSCRIPT_VALUE_P_H

#if defined(LIBSCRIPT_CONFIG_VALUEIMPL_HEADER)
#include LIBSCRIPT_CONFIG_VALUEIMPL_HEADER
#else

#include "script/array.h"
#include "script/function.h"
#include "script/lambda.h"
#include "script/object.h"
#include "script/string.h"

namespace script
{

class Value;

struct LIBSCRIPT_API CharRef
{
  String *string;
  size_t pos;
};

struct LIBSCRIPT_API ValueImpl
{
  ValueImpl(Type t, Engine *e) : ref(0), type(t), engine(e) { }

  reference_counter_type ref;
  Type type;
  Engine *engine;

  struct Storage
  {
    Storage();

    union {
      bool boolean;
      char character;
      int integer;
      float realf;
      double reald;
      String *string;
      Enumerator *enumValue;
      CharRef charref;
      Value *valueptr;
      void *data;
    }builtin;

    /// TODO : we could try to merge Lambda and Object
    // a Lambda is an object (and its members are captures)
    Value *initlistEnd;
    Object object;
    Array array;
    Function function;
    Lambda lambda;
  };
  Storage data;

  // these are part of the public interface (required by any implementation)
  inline bool get_bool() const { return data.builtin.boolean; }
  inline void set_bool(bool bval) { data.builtin.boolean = bval; }
  inline char get_char() const { return data.builtin.character; }
  inline void set_char(char cval) { data.builtin.character = cval; }
  inline int get_int() const { return data.builtin.integer; }
  inline void set_int(int ival) { data.builtin.integer = ival; }
  inline float get_float() const { return data.builtin.realf; }
  inline void set_float(float fval) { data.builtin.realf = fval; }
  inline double get_double() const { return data.builtin.reald; }
  inline void set_double(double dval) { data.builtin.reald = dval; }

  inline const String & get_string() const { return *data.builtin.string; }
  inline String & get_string() { return *data.builtin.string; }
  inline void set_string(const String & sval)
  {
    if (data.builtin.string == nullptr)
      data.builtin.string = new String{ sval };
    *data.builtin.string = sval;
  }

  bool is_object() const;
  const Object & get_object() const;
  void init_object();
  void push_member(const Value & val);
  Value pop_member();
  Value get_member(size_t i) const;
  size_t member_count() const;

  bool is_array() const;
  const Array & get_array() const;
  void set_array(const Array & aval);

  bool is_function() const;
  const Function & get_function() const;
  void set_function(const Function & fval);
  bool is_lambda() const;
  const Lambda & get_lambda() const;
  void set_lambda(const Lambda & lval);
  const Enumerator & get_enumerator() const;
  void set_enumerator(const Enumerator & val);

  bool is_initializer_list() const;
  InitializerList get_initializer_list() const;
  void set_initializer_list(const InitializerList & il);

  void clear();
};

} // namespace script

#endif // defined(LIBSCRIPT_CONFIG_VALUEIMPL_HEADER)

#endif // LIBSCRIPT_VALUE_P_H
