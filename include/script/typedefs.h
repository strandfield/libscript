// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPEDEFS_H
#define LIBSCRIPT_TYPEDEFS_H

#include "script/types.h"

#include <string>

namespace script
{

/*!
 * \class Typedef
 * \brief represents a typedef
 */

class LIBSCRIPT_API Typedef
{
public:
  Typedef() = default;
  Typedef(const Typedef&) = default;
  Typedef(Typedef&&) noexcept = default;
  ~Typedef() = default;

  Typedef(std::string name, const Type& t);

  const std::string& name() const;
  const Type& type() const;

  Typedef& operator=(const Typedef&) = default;
  Typedef& operator=(Typedef&&) noexcept = default;

private:
  std::string mName;
  Type mType;
};

/*!
 * \fn Typedef(std::string name, const Type& t)
 * \brief constructs a typedef
 */
inline Typedef::Typedef(std::string name, const Type& t)
  : mName(std::move(name))
  , mType(t)
{

}

/*!
 * \fn const std::string& name() const
 * \brief returns the name of the typedef
 */
inline const std::string& Typedef::name() const
{ 
  return mName;
}

/*!
 * \fn const Type& type() const
 * \brief returns the typedef's type
 */
inline const Type& Typedef::type() const
{ 
  return mType; 
}

inline bool operator==(const Typedef & lhs, const Typedef & rhs)
{
  return lhs.type() == rhs.type() && lhs.name() == rhs.name();
}

inline bool operator!=(const Typedef & lhs, const Typedef & rhs)
{
  return !(lhs == rhs);
}

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_TYPEDEFS_H
