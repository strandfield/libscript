// Copyright (C) 2019-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_H
#define LIBSCRIPT_VALUE_H

#include "script/types.h"
#include "script/string.h"
#include "script/value-interface.h"

#ifndef LIBSCRIPT_BUILTIN_MEMBUF_SIZE
#define LIBSCRIPT_BUILTIN_MEMBUF_SIZE 24
#endif

namespace script
{

class Array;
class Engine;
class Enumerator;
class Function;
class InitializerList;
class Lambda;
class Object;
class ThisObject;

struct ValueImpl;

class LIBSCRIPT_API Value
{
public:
  Value();
  Value(const Value& other);
  ~Value();

  explicit Value(IValue* impl);

  static constexpr ParameterPolicy Copy = ParameterPolicy::Copy;
  static constexpr ParameterPolicy Move = ParameterPolicy::Move;
  static constexpr ParameterPolicy Take = ParameterPolicy::Take;

  bool isNull() const;
  Type type() const;
  bool isConst() const;
  bool isReference() const;

  static const Value Void;

  bool isBool() const;
  bool isChar() const;
  bool isInt() const;
  bool isFloat() const;
  bool isDouble() const;
  bool isPrimitive() const;
  bool isString() const;
  //bool isObject() const;
  bool isArray() const;
  bool isInitializerList() const;

  bool toBool() const;
  char toChar() const;
  int toInt() const;
  float toFloat() const;
  double toDouble() const;
  String toString() const;
  Function toFunction() const;
  //Object toObject() const;
  Array toArray() const;
  Enumerator toEnumerator() const;
  Lambda toLambda() const;
  InitializerList toInitializerList() const;

  static constexpr size_t MemoryBufferSize = LIBSCRIPT_BUILTIN_MEMBUF_SIZE;

  void* data() const;
  void* ptr() const;

  static Value fromEnumerator(const Enumerator& ev);
  static Value fromFunction(const Function& f, const Type& ft);
  static Value fromLambda(const Lambda& obj);
  static Value fromArray(const Array& a);

  Engine* engine() const;
  bool isManaged() const;

  Value& operator=(const Value& other);

  IValue* impl() const { return d; }

private:
  IValue* d;
};

inline bool operator==(const Value& lhs, const Value& rhs) { return lhs.impl() == rhs.impl(); }
inline bool operator!=(const Value& lhs, const Value& rhs) { return !(lhs == rhs); }

} // namespace script

#include "script/value-get-details.h"

namespace script
{

template<typename T>
typename get_helper<T>::type get(const Value& val)
{
  return get_helper<T>::get(val);
}

} // namespace script

#endif // LIBSCRIPT_VALUE_H
