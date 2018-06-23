// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/prototype.h"

namespace script
{

Prototype::Prototype()
  : mCapacity(0)
  , mBegin(mParams)
  , mEnd(mParams)
{

}

Prototype::Prototype(const Prototype & other)
  : mReturnType(other.mReturnType)
{
  if (other.parameterCount() <= 4)
  {
    mBegin = mParams;
    mEnd = mParams + other.parameterCount();
    for (int i(0); i < 4; ++i)
      mParams[i] = other.mParams[i];
    mCapacity = 0;
  }
  else
  {
    mBegin = new Type[other.parameterCount()];
    mEnd = mBegin;
    for (Type t : other)
      *(mEnd++) = t;
    mCapacity = other.parameterCount();
  }
}

Prototype::Prototype(Prototype && other)
  : mReturnType(other.mReturnType)
  , mCapacity(other.mCapacity)
{
  if (other.parameterCount() <= 4)
  {
    mBegin = mParams;
    mEnd = mParams + other.parameterCount();
    for (int i(0); i < 4; ++i)
      mParams[i] = other.mParams[i];
    mCapacity = 0;
  }
  else
  {
    mBegin = other.mBegin;
    mEnd = other.mEnd;
    other.mBegin = other.mEnd = nullptr;
    other.mCapacity = 0;
  }
}

Prototype::~Prototype()
{
  if (mCapacity > 0)
    delete[] mBegin;
}

Prototype::Prototype(const Type & rt)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mParams)
  , mEnd(mBegin)
{

}

Prototype::Prototype(const Type & rt, const Type & p)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mParams)
  , mEnd(mBegin)
{
  *(mEnd++) = p;
}

Prototype::Prototype(const Type & rt, const Type & p1, const Type & p2)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mParams)
  , mEnd(mBegin)
{
  *(mEnd++) = p1;
  *(mEnd++) = p2;
}

Prototype::Prototype(const Type & rt, const Type & p1, const Type & p2, const Type & p3)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mParams)
  , mEnd(mBegin)
{
  *(mEnd++) = p1;
  *(mEnd++) = p2;
  *(mEnd++) = p3;
}

Prototype::Prototype(const Type & rt, const Type & p1, const Type & p2, const Type & p3, const Type & p4)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mParams)
  , mEnd(mBegin)
{
  *(mEnd++) = p1;
  *(mEnd++) = p2;
  *(mEnd++) = p3;
  *(mEnd++) = p4;
}

Prototype::Prototype(const Type & rt, std::initializer_list<Type> && params)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mParams)
  , mEnd(mBegin)
{
  if(params.size() > 4)
  {
    mCapacity = params.size();
    mBegin = new Type[params.size()];
    mEnd = mBegin;
  }

  for (auto it = params.begin(); it != params.end(); ++it)
    *(mEnd++) = *it;
}

void Prototype::addParameter(const Type & param)
{
  if (parameterCount() < 4 || parameterCount() < mCapacity)
    *(mEnd++) = param;
  else
  {
    const auto begin = mBegin;
    const auto end = mEnd;
    const int count = parameterCount();

    mCapacity = count == 4 ? 8 : count + 1;
    mBegin = new Type[mCapacity];
    mEnd = mBegin;

    for (auto it = begin; it != end; ++it)
      *(mEnd++) = *it;

    *(mEnd++) = param;

    if(count > 4)
      delete[] begin;
  }
}

void Prototype::popParameter()
{
  mEnd--;
  if (count() == 4)
  {
    for (int i(0); i < 4; ++i)
      mParams[i] = *(mBegin + i);

    delete[] mBegin;
    mBegin = mParams;
    mEnd = mParams + 4;
  }
}

Prototype & Prototype::operator=(const Prototype & other)
{
  mReturnType = other.returnType();

  if (other.parameterCount() <= 4)
  {
    mBegin = mParams;
    mEnd = mParams + other.parameterCount();
    for (int i(0); i < 4; ++i)
      mParams[i] = other.mParams[i];
    mCapacity = 0;
  }
  else
  {
    mBegin = new Type[other.parameterCount()];
    mEnd = mBegin;
    for (Type t : other)
      *(mEnd++) = t;
    mCapacity = other.parameterCount();
  }

  return *(this);
}

bool Prototype::operator==(const Prototype & other) const
{
  if (other.count() != this->count())
    return false;

  if (other.returnType() != mReturnType)
    return false;

  auto oit = other.begin();
  for (auto it = mBegin; it != mEnd; ++it)
  {
    if (*(oit++) != *it)
      return false;
  }

  return true;
}

} // namespace script