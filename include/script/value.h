// Copyright (C) 2019-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_H
#define LIBSCRIPT_VALUE_H

#include "script/types.h"
#include "script/string.h"
#include "script/value-interface.h"

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

  void* data() const;
  void* ptr() const;

  static Value fromEnumerator(const Enumerator& ev);
  static Value fromFunction(const Function& f, const Type& ft);
  static Value fromLambda(const Lambda& obj);
  static Value fromArray(const Array& a);

  Engine* engine() const;

  Value& operator=(const Value& other);

  IValue* impl() const { return d; }

private:
  IValue* d;
};

inline bool operator==(const Value& lhs, const Value& rhs) { return lhs.impl() == rhs.impl(); }
inline bool operator!=(const Value& lhs, const Value& rhs) { return !(lhs == rhs); }

template<typename T>
T& get(const Value& val)
{
  return *reinterpret_cast<T*>(val.ptr());
}

template<> LIBSCRIPT_API Function& get<Function>(const Value& val);
template<> LIBSCRIPT_API Array& get<Array>(const Value& val);
template<> LIBSCRIPT_API Enumerator& get<Enumerator>(const Value& val);
template<> LIBSCRIPT_API Lambda& get<Lambda>(const Value& val);
template<> LIBSCRIPT_API InitializerList& get<InitializerList>(const Value& val);

} // namespace script

#endif // LIBSCRIPT_VALUE_H
