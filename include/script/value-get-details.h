// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_GET_DETAILS_H
#define LIBSCRIPT_VALUE_GET_DETAILS_H

namespace script
{

namespace details
{

struct large_object_tag {};
struct small_object_tag {};
struct enum_tag {};

template<bool IsSmall, bool IsEnum>
struct tag_resolver_impl;

template<bool IsSmall>
struct tag_resolver_impl<IsSmall, true>
{
  typedef enum_tag type;
};

template<>
struct tag_resolver_impl<true, false>
{
  typedef small_object_tag type;
};

template<>
struct tag_resolver_impl<false, false>
{
  typedef large_object_tag type;
};

template<typename T>
struct tag_resolver
{
  typedef typename tag_resolver_impl<(sizeof(T) <= Value::MemoryBufferSize), std::is_enum<T>::value>::type tag_type;
};

struct PtrWrapper
{
  void* value;

  PtrWrapper(void* ptr) : value(ptr) { }
};

} // namespace details

template<typename T, typename Tag = typename details::tag_resolver<T>::tag_type>
struct get_helper;

template<typename T>
struct get_helper<T, details::small_object_tag>
{
  typedef T& type;

  static T& get(const script::Value& val)
  {
    return *reinterpret_cast<T*>(val.memory());
  }
};

template<typename T>
struct get_helper<T, details::large_object_tag>
{
  typedef T& type;

  static T& get(const script::Value& val)
  {
    return *static_cast<T*>(static_cast<details::PtrWrapper*>(val.memory())->value);
  }
};

template<typename T>
struct get_helper<T, details::enum_tag>
{
  typedef T type;

  static T get(const script::Value& val)
  {
    return static_cast<T>(val.toEnumerator().value());
  }
};

} // namespace script

#endif // !LIBSCRIPT_VALUE_GET_DETAILS_H
