// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functionflags.h"

namespace script
{

FunctionFlags::FunctionFlags(FunctionSpecifier val)
  : d(static_cast<int>(val) << 3)
{

}

bool FunctionFlags::test(FunctionSpecifier fs) const
{
  return (d >> 3) & static_cast<int>(fs);
}

void FunctionFlags::set(FunctionSpecifier fs)
{
  d |= (static_cast<int>(fs) << 3);
}


bool FunctionFlags::test(ImplementationMethod im) const
{
  return static_cast<ImplementationMethod>(d & 1) == im;
}

void FunctionFlags::set(ImplementationMethod im)
{
  d >>= 1;
  d = (d << 1) | static_cast<int>(im);
}

AccessSpecifier FunctionFlags::getAccess() const
{
  return static_cast<AccessSpecifier>((d >> 1) & 3);
}

void FunctionFlags::set(AccessSpecifier as)
{
  const auto im = d & 1;
  d >>= 3;
  d = (d << 3) | (static_cast<int>(as) << 1) | im;
}

} // namespace script
