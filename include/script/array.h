// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ARRAY_H
#define LIBSCRIPT_ARRAY_H

#include "value.h"

namespace script
{

class ArrayImpl;

class LIBSCRIPT_API Array
{
public:
  Array();
  Array(const Array &) = default;
  ~Array();

  Array(const std::shared_ptr<ArrayImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  Engine* engine() const;

  Type typeId() const;
  Type elementTypeId() const;

  int size() const;
  inline int length() const { return size(); }

  const Value & at(int index) const;
  Value & operator[](int index);

  void detach();

  Array & operator=(const Array &) = default;

  inline std::shared_ptr<ArrayImpl> impl() const { return d; }
  
private:
  std::shared_ptr<ArrayImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_ARRAY_H
