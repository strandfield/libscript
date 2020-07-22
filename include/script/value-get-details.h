// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_VALUE_GET_DETAILS_H
#define LIBSCRIPT_VALUE_GET_DETAILS_H

namespace script
{

namespace details
{

LIBSCRIPT_API int get_enum_value(const Value& val);

} // namespace details

template<typename T, bool IsEnum = std::is_enum<T>::value>
struct get_helper;

template<typename T>
struct get_helper<T, false>
{
  typedef T& type;

  static T& get(const script::Value& val)
  {
    return *reinterpret_cast<T*>(val.ptr());
  }
};

template<typename T>
struct get_helper<T, true>
{
  typedef T type;

  static T get(const script::Value& val)
  {
    return static_cast<T>(details::get_enum_value(val));
  }
};

} // namespace script

#endif // !LIBSCRIPT_VALUE_GET_DETAILS_H
