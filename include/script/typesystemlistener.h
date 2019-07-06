// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPESYSTEMLISTENER_H
#define LIBSCRIPT_TYPESYSTEMLISTENER_H

#include "script/types.h"

namespace script
{

class LIBSCRIPT_API TypeSystemListener
{
public:
  TypeSystemListener() = default;
  TypeSystemListener(const TypeSystemListener&) = delete;
  virtual ~TypeSystemListener() = default;

  virtual void created(const Type& t) = 0;
  virtual void destroyed(const Type& t) = 0;

  TypeSystemListener& operator=(const TypeSystemListener&) = delete;
};

} // namespace script

#endif // !LIBSCRIPT_TYPESYSTEMLISTENER_H
