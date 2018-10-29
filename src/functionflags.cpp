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
 */

/*!
 * \fn FunctionFlags(FunctionSpecifier val)
 */
FunctionFlags::FunctionFlags(FunctionSpecifier val)
  : d(static_cast<int>(val) << 3)
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
  return (d >> 3) & static_cast<int>(fs);
}

/*!
 * \fn void set(FunctionSpecifier fs)
 * \param function specifier to set
 * \brief Sets the given function specifier.
 * 
 */
void FunctionFlags::set(FunctionSpecifier fs)
{
  d |= (static_cast<int>(fs) << 3);
}

/*!
 * \fn bool test(ImplementationMethod im) const
 * \param implementation method
 * \brief Returns wether the function uses the given implementation method.
 * 
 */
bool FunctionFlags::test(ImplementationMethod im) const
{
  return static_cast<ImplementationMethod>(d & 1) == im;
}

/*!
 * \fn void set(ImplementationMethod im)
 * \param implementation method
 * \brief Sets the implementation method.
 * 
 * The previous implementation method is erased.
 */
void FunctionFlags::set(ImplementationMethod im)
{
  d >>= 1;
  d = (d << 1) | static_cast<int>(im);
}

/*!
 * \fn AccessSpecifier getAccess() const
 * \brief Returns the currently set access specifier.
 * 
 */
AccessSpecifier FunctionFlags::getAccess() const
{
  return static_cast<AccessSpecifier>((d >> 1) & 3);
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
  const auto im = d & 1;
  d >>= 3;
  d = (d << 3) | (static_cast<int>(as) << 1) | im;
}

} // namespace script
