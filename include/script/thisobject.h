// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_THISOBJECT_H
#define LIBSCRIPT_THISOBJECT_H

#include "script/value.h"

namespace script
{

class LIBSCRIPT_API ThisObject
{
public:
  ThisObject(Value& val) : m_value(val) { }
  ~ThisObject() = default;

  void init();
  void push(const Value& val);
  Value pop();
  void destroy();

  template<typename T, typename...Args>
  std::enable_if_t<std::is_same<typename details::tag_resolver<T>::tag_type, details::small_object_tag>::value, void>
    init(Args&& ... args)
  {
    void* mem = get().acquireMemory();
    new (mem) T(std::forward<Args>(args)...);
  }

  template<typename T, typename...Args>
  std::enable_if_t<std::is_same<typename details::tag_resolver<T>::tag_type, details::large_object_tag>::value, void>
    construct(Args&& ... args)
  {
    void* mem = get().acquireMemory();
    new (mem) details::PtrWrapper(new T(std::forward<Args>(args)...));
  }

  template<typename T>
  std::enable_if_t<std::is_same<typename details::tag_resolver<T>::tag_type, details::small_object_tag>::value, void>
    destroy()
  {
    T* ptr = static_cast<T*>(get().memory());
    ptr->~T();
    get().releaseMemory();
  }

  template<typename T>
  std::enable_if_t<std::is_same<typename details::tag_resolver<T>::tag_type, details::large_object_tag>::value, void>
    destroy()
  {
    T* ptr = static_cast<T*>(static_cast<details::PtrWrapper*>(get().memory())->value);
    delete ptr;
    get().releaseMemory();
  }

  operator Value& () { return m_value; }
  Value& get() { return m_value; }

private:
  Value& m_value;
};

} // namespace script

#endif // LIBSCRIPT_THISOBJECT_H
