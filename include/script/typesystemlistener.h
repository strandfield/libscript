// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPESYSTEMLISTENER_H
#define LIBSCRIPT_TYPESYSTEMLISTENER_H

#include "script/types.h"

namespace script
{

class TypeSystem;

class LIBSCRIPT_API TypeSystemListener
{
public:
  TypeSystemListener() = default;
  TypeSystemListener(const TypeSystemListener&) = delete;
  virtual ~TypeSystemListener() = default;

  TypeSystem* typeSystem() const;

  virtual void created(const Type& t) = 0;
  virtual void destroyed(const Type& t) = 0;

  TypeSystemListener& operator=(const TypeSystemListener&) = delete;

private:
  friend class TypeSystem;
  TypeSystem* m_typesystem = nullptr;
};

inline TypeSystem* TypeSystemListener::typeSystem() const
{
  return m_typesystem;
}

} // namespace script

#endif // !LIBSCRIPT_TYPESYSTEMLISTENER_H
