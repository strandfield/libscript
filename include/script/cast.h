// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIP_CAST_H
#define LIBSCRIP_CAST_H

#include "script/function.h"

namespace script
{

class CastImpl;

/*!
 * \class Cast
 * \brief represents a conversion function
 */
class LIBSCRIPT_API Cast : public Function
{
public:
  Cast() = default;
  Cast(const Cast &) = default;
  ~Cast() = default;

  [[deprecated("use more general overload")]] explicit Cast(const std::shared_ptr<CastImpl> & impl);
  Cast(const Function& f);

  Type sourceType() const;
  Type destType() const;

  Cast & operator=(const Cast &) = default;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIP_CAST_H
