// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUMVALUE_H
#define LIBSCRIPT_ENUMVALUE_H

#include "enum.h"

namespace script
{

class LIBSCRIPT_API EnumValue
{
public:
  EnumValue();
  EnumValue(const EnumValue & other) = default;
  ~EnumValue() = default;

  EnumValue(Enum e, int val);
  EnumValue(Enum e, const std::string & name);

  inline bool isNull() const { return mEnum.isNull(); }
  inline bool isValid() const { return !isNull(); }

  inline int value() const { return mValue; }
  inline const Enum & enumeration() const { return mEnum; }

  EnumValue & operator=(const EnumValue & other) = default;
  bool operator==(const EnumValue & other) const;
  bool operator!=(const EnumValue & other) const;

private:
  Enum mEnum;
  int mValue;
};

} // namespace script

#endif // LIBSCRIPT_ENUMVALUE_H
