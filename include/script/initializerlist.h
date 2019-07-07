// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_INITIALIZER_LIST_H
#define LIBSCRIPT_INITIALIZER_LIST_H

#include "libscriptdefs.h"

#include "script/classtemplatenativebackend.h"

namespace script
{

class Value;

class LIBSCRIPT_API InitializerList
{
private:
  Value *begin_;
  Value *end_;

public:
  InitializerList() : begin_(nullptr), end_(nullptr) { }
  InitializerList(Value *b, Value *e) : begin_(b), end_(e) { }
  InitializerList(const InitializerList &) = default;
  ~InitializerList() = default;

  typedef Value* iterator;

  inline iterator begin() const { return begin_; }
  inline iterator end()  const { return end_; }

  size_t size() const;

  InitializerList & operator=(const InitializerList &) = default;
};

class LIBSCRIPT_API InitializerListTemplate : public ClassTemplateNativeBackend
{
  Class instantiate(ClassTemplateInstanceBuilder& builder) override;
};

} // namespace script

#endif // LIBSCRIPT_INITIALIZER_LIST_H
