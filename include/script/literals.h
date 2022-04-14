// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LITERALS_H
#define LIBSCRIPT_LITERALS_H

#include "script/function.h"

namespace script
{

class LiteralOperatorImpl;

/*!
 * \class LiteralOperator
 */

class LIBSCRIPT_API LiteralOperator : public Function
{
public:
  LiteralOperator() = default;
  LiteralOperator(const LiteralOperator & other) = default;
  ~LiteralOperator() = default;

  [[deprecated("use more general overload")]] explicit LiteralOperator(const std::shared_ptr<LiteralOperatorImpl> & impl);
  LiteralOperator(const Function& f);

  Type input() const;
  Type output() const;

  const std::string & suffix() const;

  LiteralOperator & operator=(const LiteralOperator & other) = default;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_LITERALS_H
