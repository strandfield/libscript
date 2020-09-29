// Copyright (C) 2019-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_THISOBJECT_H
#define LIBSCRIPT_THISOBJECT_H

#include "script/value.h"
#include "script/hybrid-cpp-value.h"

namespace script
{

/*!
 * \class ThisObject
 */
class LIBSCRIPT_API ThisObject
{
public:
  ThisObject(Value& val, Engine* e) : m_value(val), m_engine(e) { }
  ~ThisObject() = default;

  void init(script::Type t);
  void push(const Value& val);
  Value pop();
  void destroy();
  
  /*!
   * \fn template<typename T, typename...Args> void init<T>(Args &&...)
   * \brief initializes this with a C++ object
   */
  template<typename T, typename...Args>
  void init(Args&& ... args)
  {
    if(std::is_class<T>::value)
      m_value = Value(new HybridCppValue<T>(m_engine, std::forward<Args>(args)...));
    else
      m_value = Value(new CppValue<T>(m_engine, std::forward<Args>(args)...));
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
