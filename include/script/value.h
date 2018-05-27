// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_H
#define LIBSCRIPT_VALUE_H

#include "libscriptdefs.h"
#include "types.h"
#include "script/string.h"

#if defined(LIBSCRIPT_CONFIG_VALUE_INJECTED_HEADER)
#include LIBSCRIPT_CONFIG_VALUE_INJECTED_HEADER
#endif // defined(LIBSCRIPT_CONFIG_VALUE_INJECTED_HEADER)

namespace script
{

class Array;
class Engine;
class EnumValue;
class Function;
class LambdaObject;
class Object;

struct ValueImpl;

class LIBSCRIPT_API Value
{
public:
  Value();
  Value(const Value & other);
  ~Value();

  Value(ValueImpl * impl);

  enum ParameterPolicy {
    Copy,
    Move,
    Take
  };

  bool isNull() const;
  Type type() const;
  bool isConst() const;
  bool isInitialized() const;

  static const Value Void;

  bool isBool() const;
  bool isChar() const;
  bool isInt() const;
  bool isFloat() const;
  bool isDouble() const;
  bool isPrimitive() const;
  bool isString() const;
  bool isObject() const;
  bool isArray() const;

  bool toBool() const;
  char toChar() const;
  int toInt() const;
  float toFloat() const;
  double toDouble() const;
  String toString() const;
  Function toFunction() const;
  Object toObject() const;
  Array toArray() const;
  EnumValue toEnumValue() const;
  LambdaObject toLambda() const;

  static Value fromEnumValue(const EnumValue & ev);
  static Value fromFunction(const Function & f, const Type & ft);
  static Value fromLambda(const LambdaObject & obj);
  static Value fromArray(const Array & a);

  Engine* engine() const;
  bool isManaged() const;

  Value & operator=(const Value & other);
  bool operator==(const Value & other) const;
  inline bool operator!=(const Value & other) const { return !operator==(other); }

  inline ValueImpl * impl() const { return d; }

#if defined(LIBSCRIPT_CONFIG_VALUE_INJECTED_METHODS)
#include LIBSCRIPT_CONFIG_VALUE_INJECTED_METHODS
#endif // defined(LIBSCRIPT_CONFIG_VALUE_INJECTED_METHODS)

private:
  ValueImpl *d;
};

} // namespace script

#endif // LIBSCRIPT_VALUE_H
