// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_P_H
#define LIBSCRIPT_VALUE_P_H

#include "script/array.h"
#include "script/object.h"
#include "script/lambda.h"

namespace script
{

struct LIBSCRIPT_API ValueStruct
{
  ValueStruct(Type t, Engine *e) : ref(0), type(t), engine(e) { }

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
      EnumValue *enumValue;
      void *data;
    }builtin;

    Object object;
    Array array;
    Function function;
    LambdaObject lambda;
  };
  Storage data;

  inline bool getBool() const { return data.builtin.boolean; }
  inline void setBool(bool bval) { data.builtin.boolean = bval; }
  inline char getChar() const { return data.builtin.character; }
  inline void setChar(char cval) { data.builtin.character = cval; }
  inline int getInt() const { return data.builtin.integer; }
  inline void setInt(int ival) { data.builtin.integer = ival; }
  inline float getFloat() const { return data.builtin.realf; }
  inline void setFloat(float fval) { data.builtin.realf = fval; }
  inline double getDouble() const { return data.builtin.reald; }
  inline void setDouble(double dval) { data.builtin.reald = dval; }
  inline const String & getString() const { return *data.builtin.string; }
  inline String & getString() { return *data.builtin.string; }
  inline void setString(const String & sval)
  {
    if (data.builtin.string == nullptr)
      data.builtin.string = new String{ sval };
    *data.builtin.string = sval;
  }
  bool isObject() const;
  const Object & getObject() const;
  void setObject(const Object & oval);
  bool isArray() const;
  const Array & getArray() const;
  void setArray(const Array & aval);
  bool isFunction() const;
  const Function & getFunction() const;
  void setFunction(const Function & fval);
  bool isLambda() const;
  const LambdaObject & getLambda() const;
  void setLambda(const LambdaObject & lval);
  const EnumValue & getEnumValue() const;
  void setEnumValue(const EnumValue & evval);
  void clear();

  inline bool isManaged() const { return this->type.testFlag(Type::ManagedFlag); }
  inline void setManaged(bool m)
  {
    if (m)
      this->type = this->type.withFlag(Type::ManagedFlag);
    else
      this->type = this->type.withoutFlag(Type::ManagedFlag);
  }
};

} // namespace script

#endif // LIBSCRIPT_VALUE_P_H
