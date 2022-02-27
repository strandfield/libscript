// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ATTRIBUTES_H
#define LIBSCRIPT_ATTRIBUTES_H

#include "libscriptdefs.h"

#include <memory>
#include <vector>

namespace script
{

namespace ast
{
class Node;
} // namespace ast

using Attribute = std::shared_ptr<ast::Node>;
using AttributeVector = std::vector<Attribute>;

/*!
 * \class Attributes
 * \brief a list of attributes
 */
class LIBSCRIPT_API Attributes
{
public:

  using iterator = AttributeVector::const_iterator;

  Attributes();
  Attributes(const Attributes&) = default;
  
  Attributes(iterator begin, iterator end);

  iterator begin() const;
  iterator end() const;

  size_t size() const;
  const Attribute& at(size_t index) const;

  Attributes& operator=(const Attributes&) = default;

private:
  iterator m_begin;
  iterator m_end;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_ATTRIBUTES_H
