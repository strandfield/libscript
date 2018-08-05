// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/enumerator.h"

namespace script
{

Enumerator::Enumerator()
  : mValue(-1)
{

}

Enumerator::Enumerator(Enum e, int val)
  : mEnum(e)
  , mValue(val)
{

}

bool Enumerator::operator==(const Enumerator & other) const
{
  return other.mEnum == mEnum && other.mValue == mValue;
}

bool Enumerator::operator!=(const Enumerator & other) const
{
  return !operator==(other);
}

} // namespace script