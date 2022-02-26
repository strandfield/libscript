// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functionflags.h"

namespace script
{

/*!
 * \enumclass FunctionSpecifier
 * \brief Enumeration describing function specifiers.
 * 
 * \begin{list}
 *   \li \c None
 *   \li \c Static
 *   \li \c Explicit
 *   \li \c Virtual
 *   \li \c Pure
 *   \li \c ConstExpr
 *   \li \c Default
 *   \li \c Delete
 * \begin{list}
 */


/*!
 * \class FunctionFlags
 * \brief Provides a type-safe OR-combination of function and access specifiers.
 *
 * The FunctionFlags is used to store flags describing a \t Function.
 * It is used to store both \t FunctionSpecifier (i.e. is the function static, or 
 * virtual, or deleted) and an \t AccessSpecifier (public, protected, private).
 * 
 * Internally, the access specifier is stored on the two first bits and 
 * the FunctionSpecifier on the remaining bits.
 */

/*!
 * \fn FunctionFlags(FunctionSpecifier val)
 */
FunctionFlags::FunctionFlags(FunctionSpecifier val)
  : d(static_cast<int>(val) << 2)
{

}

/*!
 * \fn bool test(FunctionSpecifier fs) const
 * \param function specifier to test
 * \brief Test if the given specifier is set.
 * 
 */
bool FunctionFlags::test(FunctionSpecifier fs) const
{
  return (d >> 2) & static_cast<int>(fs);
}

/*!
 * \fn void set(FunctionSpecifier fs)
 * \param function specifier to set
 * \brief Sets the given function specifier.
 * 
 */
void FunctionFlags::set(FunctionSpecifier fs)
{
  d |= (static_cast<int>(fs) << 2);
}

/*!
 * \fn AccessSpecifier getAccess() const
 * \brief Returns the currently set access specifier.
 * 
 */
AccessSpecifier FunctionFlags::getAccess() const
{
  return static_cast<AccessSpecifier>(d & 3);
}

/*!
 * \fn void set(AccessSpecifier as)
 * \param the access specifier to set
 * \brief Sets the access specifier
 * 
 * The previous access specifier is erased.
 */
void FunctionFlags::set(AccessSpecifier as)
{
  d >>= 2;
  d = (d << 2) | static_cast<int>(as);
}

} // namespace script
