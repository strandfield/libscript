// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PROTOTYPE_H
#define LIBSCRIPT_PROTOTYPE_H

#include "script/types.h"

#include <vector>

namespace script
{

/*!
 * \class Prototype
 * \brief represents a function prototype.
 * 
 * The Prototype class describes the signature of any function; that is, 
 * its return type and parameters.
 *
 * This class does not own the list of parameters: it only holds a [begin, end) 
 * range of parameters. As such, a \t Prototype cannot be copied.
 * Subclasses are expected to actually hold the list of parameters.
 */

class LIBSCRIPT_API Prototype
{
public:
  Prototype();
  Prototype(const Prototype &) = delete;
  ~Prototype();

  explicit Prototype(const Type& rt);
  Prototype(const Type& rt, Type* begin, Type* end);

  const Type& returnType() const;
  void setReturnType(const Type& rt);

  size_t parameterCount() const;
  size_t count() const;
  size_t size() const;
  const Type& parameter(size_t index) const;
  const Type& at(size_t index) const;
  std::vector<Type> parameters() const;
  const Type* begin() const;
  const Type* end() const;
  void setParameter(size_t index, const Type& t);

  Prototype& operator=(const Prototype&) = delete;
  Type& operator[](size_t index);

protected:
  inline void setParameters(Type *begin, Type *end) { mBegin = begin; mEnd = end; }

private:
  Type mReturnType;
  Type *mBegin;
  Type *mEnd;
};

/*!
 * \fn const Type& returnType() const
 * \brief returns the return type
 */
inline const Type& Prototype::returnType() const 
{ 
  return mReturnType; 
}

/*!
 * \fn void setReturnType(const Type& rt)
 * \brief sets the prototype return's type
 */
inline void Prototype::setReturnType(const Type& rt) 
{ 
  mReturnType = rt; 
}

/*!
 * \fn size_t parameterCount() const
 * \brief returns the number of parameters of the prototype
 */
inline size_t Prototype::parameterCount() const 
{ 
  return static_cast<size_t>(std::distance(mBegin, mEnd)); 
}

/*!
 * \fn size_t count() const
 * \brief returns the number of parameters of the prototype
 */
inline size_t Prototype::count() const 
{ 
  return parameterCount(); 
}

/*!
 * \fn size_t size() const
 * \brief same as count
 */
inline size_t Prototype::size() const 
{ 
  return parameterCount(); 
}

/*!
 * \fn const Type& parameter(size_t index) const
 * \brief retrieves one of the parameter of the prototype
 */
inline const Type& Prototype::parameter(size_t index) const 
{ 
  return *(mBegin + index); 
}

/*!
 * \fn const Type& at(size_t index) const
 * \brief retrieves one of the parameter of the prototype
 */
inline const Type& Prototype::at(size_t index) const 
{ 
  return parameter(index); 
}

/*!
 * \fn std::vector<Type> parameters() const
 * \brief returns the parameters as a vector
 */
inline std::vector<Type> Prototype::parameters() const 
{ 
  return std::vector<Type>{ mBegin, mEnd }; 
}

/*!
 * \fn const Type* begin() const
 * \brief returns a pointer to the first parameter
 */
inline const Type* Prototype::begin() const 
{ 
  return mBegin; 
}

/*!
 * \fn const Type* end() const
 * \brief returns a pointer after the last parameter
 */
inline const Type* Prototype::end() const 
{ 
  return mEnd; 
}

/*!
 * \fn void setParameter(size_t index, const Type& t)
 * \brief set one parameter
 */
inline void Prototype::setParameter(size_t index, const Type& t) 
{ 
  *(mBegin + index) = t; 
}

/*!
 * \fn Type& operator[](size_t index)
 * \brief access a parameter by its index
 */
inline Type& Prototype::operator[](size_t index) 
{ 
  return *(mBegin + index); 
}

/*!
 * \endclass
 */

LIBSCRIPT_API bool operator==(const Prototype& lhs, const Prototype& rhs);

inline bool operator!=(const Prototype& lhs, const Prototype& rhs) 
{ 
  return !(lhs == rhs); 
}

} // namespace script

#endif // LIBSCRIPT_PROTOTYPE_H
