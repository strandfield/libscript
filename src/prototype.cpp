// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/prototype.h"

namespace script
{

Prototype::Prototype()
  : mCapacity(0)
  , mBegin(mArgs)
  , mEnd(mArgs)
{

}
Prototype::Prototype(const Prototype & other)
  : mReturnType(other.mReturnType)
{
  if (other.argumentCount() <= 4)
  {
    mBegin = mArgs;
    mEnd = mArgs + other.argumentCount();
    for (int i(0); i < 4; ++i)
      mArgs[i] = other.mArgs[i];
    mCapacity = 0;
  }
  else
  {
    mBegin = new Type[other.argumentCount()];
    mEnd = mBegin;
    for (Type t : other)
      *(mEnd++) = t;
    mCapacity = other.argumentCount();
  }
}

Prototype::Prototype(Prototype && other)
  : mReturnType(other.mReturnType)
  , mCapacity(other.mCapacity)
{
  if (other.argumentCount() <= 4)
  {
    mBegin = mArgs;
    mEnd = mArgs + other.argumentCount();
    for (int i(0); i < 4; ++i)
      mArgs[i] = other.mArgs[i];
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
  , mBegin(mArgs)
  , mEnd(mBegin)
{

}

Prototype::Prototype(const Type & rt, const Type & arg)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mArgs)
  , mEnd(mBegin)
{
  *(mEnd++) = arg;
}

Prototype::Prototype(const Type & rt, const Type & arg1, const Type & arg2)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mArgs)
  , mEnd(mBegin)
{
  *(mEnd++) = arg1;
  *(mEnd++) = arg2;
}

Prototype::Prototype(const Type & rt, const Type & arg1, const Type & arg2, const Type & arg3)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mArgs)
  , mEnd(mBegin)
{
  *(mEnd++) = arg1;
  *(mEnd++) = arg2;
  *(mEnd++) = arg3;
}

Prototype::Prototype(const Type & rt, const Type & arg1, const Type & arg2, const Type & arg3, const Type & arg4)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mArgs)
  , mEnd(mBegin)
{
  *(mEnd++) = arg1;
  *(mEnd++) = arg2;
  *(mEnd++) = arg3;
  *(mEnd++) = arg4;
}

Prototype::Prototype(const Type & rt, std::initializer_list<Type> && args)
  : mReturnType(rt)
  , mCapacity(0)
  , mBegin(mArgs)
  , mEnd(mBegin)
{
  if(args.size() > 4)
  {
    mCapacity = args.size();
    mBegin = new Type[args.size()];
    mEnd = mBegin;
  }

  for (auto it = args.begin(); it != args.end(); ++it)
    *(mEnd++) = *it;
}

const Type & Prototype::argumentType(int index) const
{
  return *(mBegin + index);
}

void Prototype::addArgument(const Type & arg)
{
  if (argumentCount() < 4 || argumentCount() < mCapacity)
    *(mEnd++) = arg;
  else
  {
    const auto begin = mBegin;
    const auto end = mEnd;
    const int count = argumentCount();

    mCapacity = count == 4 ? 8 : count + 1;
    mBegin = new Type[mCapacity];
    mEnd = mBegin;

    for (auto it = begin; it != end; ++it)
      *(mEnd++) = *it;

    *(mEnd++) = arg;

    if(count > 4)
      delete[] begin;
  }
}

void Prototype::popArgument()
{
  mEnd--;
  if (argc() == 4)
  {
    for (int i(0); i < 4; ++i)
      mArgs[i] = *(mBegin + i);

    delete[] mBegin;
    mBegin = mArgs;
    mEnd = mArgs + 4;
  }
}

std::vector<Type> Prototype::arguments() const
{
  return std::vector<Type>{mBegin, mEnd};
}

const Type * Prototype::begin() const
{
  return mBegin;
}

const Type * Prototype::end() const
{
  return mEnd;
}

void Prototype::setParameter(int index, const Type & t)
{
  *(mBegin + index) = t;
}

Prototype & Prototype::operator=(const Prototype & other)
{
  mReturnType = other.returnType();

  if (other.argumentCount() <= 4)
  {
    mBegin = mArgs;
    mEnd = mArgs + other.argumentCount();
    for (int i(0); i < 4; ++i)
      mArgs[i] = other.mArgs[i];
    mCapacity = 0;
  }
  else
  {
    mBegin = new Type[other.argumentCount()];
    mEnd = mBegin;
    for (Type t : other)
      *(mEnd++) = t;
    mCapacity = other.argumentCount();
  }

  return *(this);
}

bool Prototype::operator==(const Prototype & other) const
{
  if (other.argc() != this->argc())
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

bool Prototype::operator!=(const Prototype & other) const
{
  return !operator==(other);
}

} // namespace script