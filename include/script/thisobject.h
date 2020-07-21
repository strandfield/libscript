// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_THISOBJECT_H
#define LIBSCRIPT_THISOBJECT_H

#include "script/value.h"

#include "script/private/value_p.h"

namespace script
{

class LIBSCRIPT_API ThisObject
{
public:
  ThisObject(Value& val, Engine* e) : m_value(val), m_engine(e) { }
  ~ThisObject() = default;

  void init();
  void push(const Value& val);
  Value pop();
  void destroy();

  template<typename T, typename...Args>
  void init(Args&& ... args)
  {
    m_value = Value(new CppValue<T>(m_engine, T(std::forward<Args>(args)...)));
  }

  template<typename T>
  void destroy()
  {
    m_value = Value::Void;
  }

  operator Value& () { return m_value; }
  Value& get() { return m_value; }

  ThisObject& operator=(const Value& val)
  {
    m_value = val;
    return *this;
  }

private:
  Value& m_value;
  Engine* m_engine;
};

} // namespace script

#endif // LIBSCRIPT_THISOBJECT_H
