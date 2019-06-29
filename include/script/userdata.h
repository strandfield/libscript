// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIP_USERDATA_H
#define LIBSCRIP_USERDATA_H

#include "libscriptdefs.h"

namespace script
{

class LIBSCRIPT_API UserData
{
public:
  UserData() = default;
  UserData(const UserData &) = delete;
  virtual ~UserData() = default;

  UserData & operator=(const UserData &) = delete;
};

template<typename T>
class GenericUserData : public UserData
{
public:
  T value;
public:

  template<typename...Args>
  GenericUserData(Args&& ... args) : value(std::forward<Args>(args)...) { }

  ~GenericUserData() = default;
};

template<typename T, typename...Args>
std::shared_ptr<UserData> make_userdata(Args &&... args)
{
  return std::make_shared<GenericUserData<T>>(std::forward<Args>(args)...);
}

} // namespace script

#endif // LIBSCRIP_USERDATA_H
