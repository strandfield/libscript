// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DEFAULTARGUMENTS_H
#define LIBSCRIPT_DEFAULTARGUMENTS_H

#include "libscriptdefs.h"

#include <memory>
#include <vector>

namespace script
{

namespace program
{
class Expression;
} // namespace program

using DefaultArgument = std::shared_ptr<program::Expression>;
using DefaultArgumentVector = std::vector<DefaultArgument>;

/*!
 * \class DefaultArguments
 * \brief a list of default arguments
 */
class LIBSCRIPT_API DefaultArguments
{
public:

  using iterator = DefaultArgumentVector::const_iterator;

  DefaultArguments();
  DefaultArguments(const DefaultArguments&) = default;
  
  DefaultArguments(iterator begin, iterator end);

  iterator begin() const;
  iterator end() const;

  size_t size() const;
  const DefaultArgument& at(size_t index) const;

  DefaultArguments& operator=(const DefaultArguments&) = default;

private:
  iterator m_begin;
  iterator m_end;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_DEFAULTARGUMENTS_H
