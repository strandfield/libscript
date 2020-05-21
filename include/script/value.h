// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_H
#define LIBSCRIPT_VALUE_H

#include "script/types.h"
#include "script/string.h"

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

  explicit Value(ValueImpl* impl);

  static constexpr ParameterPolicy Copy = ParameterPolicy::Copy;
  static constexpr ParameterPolicy Move = ParameterPolicy::Move;
  static constexpr ParameterPolicy Take = ParameterPolicy::Take;

  bool isNull() const;
  Type type() const;
  bool isConst() const;

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

  static constexpr size_t MemoryBufferSize = LIBSCRIPT_BUILTIN_MEMBUF_SIZE;

  void* memory() const;

  void* data() const;

  static Value fromEnumerator(const Enumerator& ev);
  static Value fromFunction(const Function& f, const Type& ft);
  static Value fromLambda(const Lambda& obj);
  static Value fromArray(const Array& a);

  Engine* engine() const;
  bool isManaged() const;

  Value& operator=(const Value& other);
  bool operator==(const Value& other) const;
  inline bool operator!=(const Value& other) const { return !operator==(other); }

  inline ValueImpl* impl() const { return d; }

protected:
  friend Engine;
  friend ThisObject;

  void* acquireMemory();
  void releaseMemory();

private:
  ValueImpl* d;
};

} // namespace script

#include "script/value-get-details.h"

namespace script
{

template<typename T>
typename get_helper<T>::type get(const Value& val)
{
  return get_helper<T>::get(val);
}

/* get<T>() specializations */

template<> LIBSCRIPT_API bool& get<bool>(const Value& val);
template<> LIBSCRIPT_API char& get<char>(const Value& val);
template<> LIBSCRIPT_API int& get<int>(const Value& val);
template<> LIBSCRIPT_API float& get<float>(const Value& val);
template<> LIBSCRIPT_API double& get<double>(const Value& val);
template<> LIBSCRIPT_API String& get<String>(const Value& val);

} // namespace script

#endif // LIBSCRIPT_VALUE_H
