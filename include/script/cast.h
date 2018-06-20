// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIP_CAST_H
#define LIBSCRIP_CAST_H

#include "script/function.h"

namespace script
{

class CastImpl;

class LIBSCRIPT_API Cast : public Function
{
public:
  Cast() = default;
  Cast(const Cast & other) = default;
  ~Cast() = default;

  Cast(const std::shared_ptr<CastImpl> & impl);

  Type sourceType() const;
  Type destType() const;

  Cast & operator=(const Cast & other) = default;

  std::shared_ptr<CastImpl> impl() const;
};

} // namespace script

#endif // LIBSCRIP_CAST_H
