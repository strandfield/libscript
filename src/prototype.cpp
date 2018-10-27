// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/prototypes.h"

namespace script
{

/*!
 * \class Prototype
 * \brief Represents a function prototype.
 * 
 * The Prototype class describes the signature of any function; that is, 
 * its return type and parameters.
 *
 * This class does not own the list of parameters: it only holds a [begin, end) 
 * range of parameters. As such, a \t Prototype cannot be copied.
 * Subclasses are expected to actually hold the list of parameters.
 */

Prototype::Prototype()
  : mReturnType(Type::Void)
  , mBegin(nullptr)
  , mEnd(nullptr)
{

}

//Prototype::Prototype(const Prototype & other)
//  : mReturnType(other.mReturnType)
//  , mBegin(other.mBegin)
//  , mEnd(other.mEnd)
//{
//
//}

Prototype::~Prototype()
{
  mBegin = nullptr;
  mEnd = nullptr;
}

Prototype::Prototype(const Type & rt)
  : mReturnType(rt)
  , mBegin(nullptr)
  , mEnd(nullptr)
{

}

Prototype::Prototype(const Type & rt, Type *begin, Type *end)
  : mReturnType(rt)
  , mBegin(begin)
  , mEnd(end)
{

}

//Prototype & Prototype::operator=(const Prototype & other)
//{
//  mReturnType = other.returnType();
//  mBegin = other.mBegin;
//  mEnd = other.mEnd;
//
//  return *(this);
//}

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



DynamicPrototype::DynamicPrototype(const DynamicPrototype & other)
  : Prototype{other.returnType()}
  , mParameters(other.mParameters)
{
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
}

DynamicPrototype::DynamicPrototype(DynamicPrototype && other)
  : Prototype{ other.returnType() }
  , mParameters(std::move(other.mParameters))
{
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
  other.setParameters(nullptr, nullptr);
}

DynamicPrototype::DynamicPrototype(const Prototype & other)
  : Prototype{ other.returnType() }
  , mParameters(other.parameters())
{
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
}

DynamicPrototype::DynamicPrototype(const Type & rt, std::vector<Type> && params)
  : Prototype(rt)
{
  mParameters = std::move(params);
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
}

void DynamicPrototype::push(const Type & p)
{
  mParameters.push_back(p);
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
}

Type DynamicPrototype::pop()
{
  Type ret = mParameters.back();
  mParameters.pop_back();
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
  return ret;
}

void DynamicPrototype::set(size_t i, const Type & p)
{
  mParameters[i] = p;
}

void DynamicPrototype::clear()
{
  mParameters.clear();
  setParameters(nullptr, nullptr);
}

void DynamicPrototype::set(std::vector<Type> && params)
{
  mParameters = std::move(params);
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
}

DynamicPrototype & DynamicPrototype::operator=(const Prototype & other)
{
  setReturnType(other.returnType());
  mParameters = other.parameters();
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
  return *this;
}

DynamicPrototype & DynamicPrototype::operator=(DynamicPrototype && other)
{
  setReturnType(other.returnType());
  mParameters = std::move(other.mParameters);
  setParameters(mParameters.data(), mParameters.data() + mParameters.size());
  other.setParameters(nullptr, nullptr);
  return *this;
}

} // namespace script