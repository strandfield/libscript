// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OBJECT_H
#define LIBSCRIPT_OBJECT_H

#include "script/value.h"
#include "script/class.h" /// TODO: replace by forward declaration

#include <utility>

namespace script
{

class ObjectImpl;

class LIBSCRIPT_API Object
{
public:
  Object() = default;
  Object(const Object & other) = default;
  ~Object() = default;

  explicit Object(const std::shared_ptr<ObjectImpl> & impl);

  bool isNull() const;
  Class instanceOf() const;

  void push(const Value & val);
  Value pop();
  const Value & at(size_t i) const;
  size_t size() const;

  Value get(const std::string & attrName) const;

  void setUserData(const std::shared_ptr<UserData>& data);
  const std::shared_ptr<UserData>& getUserData() const;

  template<typename T, typename...Args>
  void setUserData(Args&& ... args)
  {
    this->setUserData(script::make_userdata<T>(T(std::forward<Args>(args)...)));
  }

  template<typename T>
  T& getUserData() const
  {
    GenericUserData<T>* ud = dynamic_cast<GenericUserData<T>*>(getUserData().get());
    assert(ud != nullptr);
    return ud->value;
  }

  Engine* engine() const;

  static Object create(const Class & c);

  inline const std::shared_ptr<ObjectImpl> & impl() const { return d; }

  Object & operator=(const Object & other) = default;
  bool operator==(const Object & other) const;
  inline bool operator!=(const Object & other) const { return !operator==(other); }

private:
  std::shared_ptr<ObjectImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_OBJECT_H
