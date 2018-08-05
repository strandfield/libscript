// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/enumvalue.h"

namespace script
{

EnumValue::EnumValue()
  : mValue(-1)
{

}

EnumValue::EnumValue(Enum e, int val)
  : mEnum(e)
  , mValue(val)
{

}

bool EnumValue::operator==(const EnumValue & other) const
{
  return other.mEnum == mEnum && other.mValue == mValue;
}

bool EnumValue::operator!=(const EnumValue & other) const
{
  return !operator==(other);
}

} // namespace script