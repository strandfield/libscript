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
  /// TODO: remove this, or throw, do something !
  if (!mEnum.hasValue(val))
  {
    mEnum = Enum{};
    mValue = -1;
  }
}

EnumValue::EnumValue(Enum e, const std::string & name)
  : mEnum(e)
{
  /// TODO: remove this, or throw, do something !
  if (!mEnum.hasKey(name))
  {
    mEnum = Enum{};
    mValue = -1;
  }
  else
  {
    mValue = mEnum.getValue(name);
  }
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