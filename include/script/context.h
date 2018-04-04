// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONTEXT_H
#define LIBSCRIPT_CONTEXT_H

#include <map>
#include <memory>
#include <string>

#include "value.h"

namespace script
{

class ContextImpl;

class LIBSCRIPT_API Context
{
public:
  Context() = default;
  Context(const Context & other) = default;
  ~Context() = default;

  Context(const std::shared_ptr<ContextImpl> & impl);

  int id() const;
  bool isNull() const;

  Engine * engine() const;

  const std::string & name() const;
  void setName(const std::string & name);

  const std::map<std::string, Value> & vars() const;
  void addVar(const std::string & name, const Value & val);
  bool exists(const std::string & name) const;
  Value get(const std::string & name) const;

  void clear();

  ContextImpl * implementation() const;
  std::weak_ptr<ContextImpl> weakref() const;

private:
  std::shared_ptr<ContextImpl> d;
};

} // namespace script


#endif // LIBSCRIPT_CONTEXT_H
