// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/types.h"

namespace script
{


Type::Type()
  : d(0)
{

}

Type::Type(int baseType, int flags)
  : d(baseType | flags)
{

}

Type Type::baseType() const
{
  return Type{ d & 0xFFFFF };
}

bool Type::isConst() const
{
  return d & ConstFlag;
}

void Type::setConst(bool on)
{
  if (on)
    d = d | ConstFlag;
  else
    d = d & ~(ConstFlag);
}

bool Type::isReference() const
{
  return d & ReferenceFlag;
}

void Type::setReference(bool on)
{
  if (on)
    d = d | ReferenceFlag;
  else
    d = d & ~(ReferenceFlag);
}

bool Type::isRefRef() const
{
  return d & ForwardReferenceFlag;
}

bool Type::isConstRef() const
{
  return isConst() && isReference();
}

Type Type::withConst() const
{
  return this->withFlag(Type::ConstFlag);
}

Type Type::withoutConst() const
{
  return this->withoutFlag(Type::ConstFlag);
}

Type Type::withoutRef() const
{
  return this->withoutFlag(Type::ReferenceFlag).withoutFlag(Type::ForwardReferenceFlag);
}

bool Type::isFundamentalType() const
{
  return (d & 0xFFFFF) <= static_cast<int>(Double);
}

bool Type::isObjectType() const
{
  return d & ObjectFlag;
}

bool Type::isEnumType() const
{
  return d & EnumFlag;
}

bool Type::isClosureType() const
{
  return d & LambdaFlag;
}

bool Type::isFunctionType() const
{
  return d & PrototypeFlag;
}

bool Type::testFlag(TypeFlag flag) const
{
  return d & flag;
}

void Type::setFlag(TypeFlag flag)
{
  d = d | flag;
}

Type Type::withFlag(TypeFlag flag) const
{
  return Type{ d | flag };
}

Type Type::withoutFlag(TypeFlag flag) const
{
  return Type{ d & ~flag };
}

Type Type::ref(const Type & base)
{
  return Type{ base.data() | ReferenceFlag };
}

Type Type::cref(const Type & base)
{
  return Type{ base.data() | ReferenceFlag | ConstFlag };
}

Type Type::rref(const Type & base)
{
  return Type{ base.data() | ForwardReferenceFlag };
}


bool Type::operator==(const Type & other) const
{
  return ((ThisFlag - 1) & d) == ((ThisFlag - 1) & other.d);
}

bool Type::operator==(const BuiltInType & rhs) const
{
  return Type{ rhs } == *this;
}

int Type::data() const
{
  return d;
}

} // namespace script