// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_INTERFACE_H
#define LIBSCRIPT_VALUE_INTERFACE_H

#include "script/types.h"

namespace script
{

class Engine;
class Value;

class LIBSCRIPT_API IValue
{
public:
  Type type;
  Engine* engine;
  size_t ref;

public:

  IValue()
    : engine(nullptr), ref(0)
  {

  }

  IValue(Type t, Engine* e)
    : type(t), engine(e), ref(0)
  {

  }

  virtual ~IValue();

  virtual void* ptr() = 0;

  virtual bool is_void() const;
  virtual bool is_reference() const;
  virtual bool is_function() const;
  virtual bool is_lambda() const;
  virtual bool is_array() const;
  virtual bool is_initializer_list() const;
  virtual bool is_enumerator() const;

  virtual bool is_cpp_enum() const;
  virtual int get_cpp_enum_value() const;

  virtual size_t size() const; // numbers of members
  virtual void push(const Value& val);
  virtual Value pop();
  virtual Value& at(size_t index);
};

namespace details
{

template<typename T>
typename std::enable_if<std::is_enum<T>::value, int>::type get_enum_value(const T& val)
{
  return static_cast<int>(val);
}

template<typename T>
typename std::enable_if<!std::is_enum<T>::value, int>::type get_enum_value(const T&)
{
  return -1;
}

} // namespace details



template<typename T>
class CppValue : public IValue
{
public:
  T value;

public:
  ~CppValue() = default;

  template<typename...Args>
  CppValue(script::Engine* e, Args &&... args)
    : IValue(e->makeType<T>(), e),
      value(std::forward<Args>(args)...)
  {

  }

  CppValue(script::Engine* e, script::Type t, T val)
    : IValue(t, e), 
      value(std::move(val))
  {

  }

  void* ptr() override { return &value; }

  bool is_cpp_enum() const override { return std::is_enum<T>::value; }
  int get_cpp_enum_value() const override { return details::get_enum_value(value); }
};

template<typename T>
class CppReferenceValue : public IValue
{
public:
  T& reference;

public:
  CppReferenceValue(script::Engine* e, T& r)
    : IValue(e->makeType<T&>(), e), 
      reference(r)
  {

  }

  CppReferenceValue(script::Engine* e, script::Type t, T& r)
    : IValue(t, e),
      reference(r)
  {

  }

  ~CppReferenceValue() = default;

  bool is_reference() const override { return true; }
  void* ptr() override { return &reference; }

  bool is_cpp_enum() const override { return std::is_enum<T>::value; }
  int get_cpp_enum_value() const override { return details::get_enum_value(reference); }
};

} // namespace script

#endif // LIBSCRIPT_VALUE_INTERFACE_H
