// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/types.h"

namespace script
{

/*!
 * \class Type
 */

/*!
 * \fn Type()
 * \brief constructs an invalid type
 */
Type::Type()
  : d(0)
{

}

/*!
 * \fn Type(int baseType, int flags)
 * \brief constructs a type from an id and flags
 */
Type::Type(int baseType, int flags)
  : d(baseType | flags)
{

}

/*!
 * \fn bool isValid() const
 * \brief returns whether the type is valid
 */
bool Type::isValid() const
{
  const int masked = d & categoryMask();
  return !isNull() && (masked & (masked - 1)) == 0;
}

/*!
 * \fn Type baseType() const
 * \brief returns the base type
 * 
 * This function returns the type without const and reference.
 */
Type Type::baseType() const
{
  return Type{ d & 0xFFFFF };
}

/*!
 * \fn bool isConst() const
 * \brief returns whether the type is const
 */
bool Type::isConst() const
{
  return d & ConstFlag;
}

/*!
 * \fn void setConst(bool on)
 * \brief sets whether the type is const
 */
void Type::setConst(bool on)
{
  if (on)
    d = d | ConstFlag;
  else
    d = d & ~(ConstFlag);
}

/*!
 * \fn bool isReference() const
 * \brief returns whether the type is a reference
 * 
 * This function returns true only for simple references, it returns 
 * false for forwarding references.
 */
bool Type::isReference() const
{
  return d & ReferenceFlag;
}

/*!
 * \fn void setReference(bool on)
 * \brief sets whether the type is a reference
 */
void Type::setReference(bool on)
{
  if (on)
    d = d | ReferenceFlag;
  else
    d = d & ~(ReferenceFlag);
}

/*!
 * \fn bool isRefRef() const
 * \brief returns whether the type is a forwarding reference
 * 
 * This function returns false for simple references.
 */
bool Type::isRefRef() const
{
  return d & ForwardReferenceFlag;
}

/*!
 * \fn bool isConstRef() const
 * \brief returns whether the type is a const reference
 */
bool Type::isConstRef() const
{
  return isConst() && isReference();
}

/*!
 * \fn Type withConst() const
 * \brief returns this type with const
 */
Type Type::withConst() const
{
  return this->withFlag(Type::ConstFlag);
}

/*!
 * \fn Type withoutConst() const
 * \brief returns this type without const
 */
Type Type::withoutConst() const
{
  return this->withoutFlag(Type::ConstFlag);
}

/*!
 * \fn Type withoutRef() const
 * \brief returns this type without reference
 * 
 * This function removes both simple and forwarding references.
 * 
 * @TODO: not consistent with isRef().
 */
Type Type::withoutRef() const
{
  return this->withoutFlag(Type::ReferenceFlag).withoutFlag(Type::ForwardReferenceFlag);
}

/*!
 * \fn bool isFundamentalType() const
 * \brief returns whether the type is a fundamental type
 * 
 * Fundamental types are bool, char, int, float, double.
 * 
 * This function ignores const and references.
 */
bool Type::isFundamentalType() const
{
  return (d & 0xFFFFF) <= static_cast<int>(Double);
}

/*!
 * \fn bool isObjectType() const
 * \brief returns whether the type is a class
 */
bool Type::isObjectType() const
{
  return d & ObjectFlag;
}

/*!
 * \fn bool isEnumType() const
 * \brief returns whether the type is an enum
 */
bool Type::isEnumType() const
{
  return d & EnumFlag;
}

/*!
 * \fn bool isClosureType() const
 * \brief returns whether the type is a lambda
 */
bool Type::isClosureType() const
{
  return d & LambdaFlag;
}

/*!
 * \fn bool isFunctionType() const
 * \brief returns whether the type is a function type
 */
bool Type::isFunctionType() const
{
  return d & PrototypeFlag;
}

/*!
 * \fn bool testFlag(TypeFlag flag) const
 * \brief test whether a flag is set
 */
bool Type::testFlag(TypeFlag flag) const
{
  return d & flag;
}

/*!
 * \fn void setFlag(TypeFlag flag)
 * \brief sets a flag
 */
void Type::setFlag(TypeFlag flag)
{
  d = d | flag;
}

/*!
 * \fn Type withFlag(TypeFlag flag) const
 * \brief returns this type with an additional flag
 */
Type Type::withFlag(TypeFlag flag) const
{
  return Type{ d | flag };
}

/*!
 * \fn Type withoutFlag(TypeFlag flag) const
 * \brief returns this type with a flag removed
 */
Type Type::withoutFlag(TypeFlag flag) const
{
  return Type{ d & ~flag };
}

/*!
 * \fn static Type ref(const Type& base)
 * \brief creates a reference type
 */
Type Type::ref(const Type & base)
{
  return Type{ base.data() | ReferenceFlag };
}

/*!
 * \fn static Type cref(const Type& base)
 * \brief creates a const-reference type
 */
Type Type::cref(const Type & base)
{
  return Type{ base.data() | ReferenceFlag | ConstFlag };
}

/*!
 * \fn static Type rref(const Type& base)
 * \brief creates a forwarding reference type
 */
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

/*!
 * \fn int data() const
 * \brief returns the internal storage data
 */
int Type::data() const
{
  return d;
}

/*!
 * \endclass
 */

} // namespace script