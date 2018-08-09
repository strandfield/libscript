// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUMERATOR_H
#define LIBSCRIPT_ENUMERATOR_H

#include "script/enum.h"

namespace script
{

class LIBSCRIPT_API Enumerator
{
public:
  Enumerator();
  Enumerator(const Enumerator & other) = default;
  ~Enumerator() = default;

  Enumerator(Enum e, int val);

  inline bool isNull() const { return mEnum.isNull(); }
  inline bool isValid() const { return !isNull(); }

  inline int value() const { return mValue; }
  inline const Enum & enumeration() const { return mEnum; }

  inline const std::string & name() const { return mEnum.getKey(mValue); }

  Enumerator & operator=(const Enumerator & other) = default;
  bool operator==(const Enumerator & other) const;
  bool operator!=(const Enumerator & other) const;

private:
  Enum mEnum;
  int mValue;
};

} // namespace script

#endif // LIBSCRIPT_ENUMERATOR_H
