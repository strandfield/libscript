// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_H
#define LIBSCRIPT_VALUE_H

#include "script/types.h"
#include "script/string.h"

#if defined(LIBSCRIPT_HAS_CONFIG)
#include "config/libscript/value.h"
#endif // defined(LIBSCRIPT_HAS_CONFIG)

namespace script
{

class Array;
class Engine;
class Enumerator;
class Function;
class InitializerList;
class Lambda;
class Object;

struct ValueImpl;

class LIBSCRIPT_API Value
{
public:
  Value();
  Value(const Value & other);
  ~Value();

  explicit Value(ValueImpl * impl);

  static constexpr ParameterPolicy Copy = ParameterPolicy::Copy;
  static constexpr ParameterPolicy Move = ParameterPolicy::Move;
  static constexpr ParameterPolicy Take = ParameterPolicy::Take;

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
  bool isInitializerList() const;

  bool toBool() const;
  char toChar() const;
  int toInt() const;
  float toFloat() const;
  double toDouble() const;
  String toString() const;
  Function toFunction() const;
  Object toObject() const;
  Array toArray() const;
  Enumerator toEnumerator() const;
  Lambda toLambda() const;
  InitializerList toInitializerList() const;

  size_t dataMemberCount() const;
  Value getDataMember(size_t i) const;
   
  static Value fromEnumerator(const Enumerator & ev);
  static Value fromFunction(const Function & f, const Type & ft);
  static Value fromLambda(const Lambda & obj);
  static Value fromArray(const Array & a);

  Engine* engine() const;
  bool isManaged() const;

  Value & operator=(const Value & other);
  bool operator==(const Value & other) const;
  inline bool operator!=(const Value & other) const { return !operator==(other); }

  inline ValueImpl * impl() const { return d; }

#if defined(LIBSCRIPT_HAS_CONFIG)
#include "config/libscript/value-members.incl"
#endif // defined(LIBSCRIPT_HAS_CONFIG)

private:
  ValueImpl *d;
};

} // namespace script

#endif // LIBSCRIPT_VALUE_H
