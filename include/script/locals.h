// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LOCALS_H
#define LIBSCRIPT_LOCALS_H

#include <vector>

#include "script/value.h"

namespace script
{

class LocalsProxy;

// @TODO: remove this class, it is no longer usefull
class LIBSCRIPT_API Locals
{
public:
  Locals() = default;
  Locals(const Locals& ) = delete;
  Locals(Locals&& other);
  ~Locals();

  void push(const Value& val);
  void pop();

  Value take();

  size_t size() const;
  const Value& at(size_t index) const;

  const std::vector<Value>& data() const;

  Locals& operator=(const Locals& ) = delete;
  Locals& operator=(Locals&& other);

  LocalsProxy operator[](size_t index);
  const Value& operator[](size_t index) const;

protected:
  void destroy();

private:
  friend class LocalsProxy;
  std::vector<Value> m_values;
};

class LIBSCRIPT_API LocalsProxy
{
public:
  Locals* l;
  size_t i;

public:
  LocalsProxy(Locals& locals, size_t index);
  LocalsProxy(const LocalsProxy&) = default;
  ~LocalsProxy() = default;

  LocalsProxy& operator=(const LocalsProxy& other);
  LocalsProxy& operator=(const Value& other);

  operator const Value& () const;
};

} // namespace script

#endif // LIBSCRIPT_LOCALS_H
