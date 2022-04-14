// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/defaultarguments.h"
#include "script/defaultargumentsmap.h"

namespace script
{

static const DefaultArgumentVector gDefaultArgumentVector = {};

/*!
 * \class DefaultArguments
 */

/*!
 * \fn DefaultArguments()
 * \brief constructs an empty list of default arguments
 */
DefaultArguments::DefaultArguments()
  : DefaultArguments(gDefaultArgumentVector.begin(), gDefaultArgumentVector.end())
{

}

/*!
 * \fn DefaultArguments(iterator begin, iterator end)
 * \brief constructs a list of default arguments from a pair of iterators
 */
DefaultArguments::DefaultArguments(iterator begin, iterator end)
  : m_begin(begin),
    m_end(end)
{

}

/*!
 * \fn DefaultArguments::iterator begin() const
 * \brief returns the begin iterator
 */
DefaultArguments::iterator DefaultArguments::begin() const
{
  return m_begin;
}

/*!
 * \fn DefaultArguments::iterator end() const
 * \brief returns the end iterator
 */
DefaultArguments::iterator DefaultArguments::end() const
{
  return m_end;
}

/*!
 * \fn size_t size() const
 * \brief returns number of arguments in the list
 */
size_t DefaultArguments::size() const
{
  return std::distance(begin(), end());
}

/*!
 * \fn const DefaultArgument& at(size_t index) const
 * \brief returns the argument at a given index
 */
const DefaultArgument& DefaultArguments::at(size_t index) const
{
  return *(begin() + index);
}

/*!
 * \endclass
 */


 /*!
  * \class DefaultArgumentsMap
  */

void DefaultArgumentsMap::add(const void* elem, const DefaultArgumentVector& dargs)
{
  if (dargs.empty())
    return;

  Span span;
  span.first = m_data.size();
  span.second = span.first + dargs.size();

  m_data.insert(m_data.end(), dargs.begin(), dargs.end());
  m_map[elem] = span;
}

DefaultArguments DefaultArgumentsMap::get(const void* elem) const
{
  auto it = m_map.find(elem);

  if (it == m_map.end())
    return DefaultArguments();

  Span span = it->second;
  return DefaultArguments(m_data.begin() + span.first, m_data.begin() + span.second);
}

void DefaultArgumentsMap::clear()
{
  m_map.clear();
  m_data.clear();
}

/*!
 * \endclass
 */

} // namespace script
