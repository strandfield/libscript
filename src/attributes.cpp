// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/attributes.h"
#include "script/attributes-map.h"

namespace script
{

static const AttributeVector gDefaultAttributeVector = {};

/*!
 * \class Attributes
 */

Attributes::Attributes()
  : Attributes(gDefaultAttributeVector.begin(), gDefaultAttributeVector.end())
{

}

Attributes::Attributes(iterator begin, iterator end)
  : m_begin(begin),
    m_end(end)
{

}

Attributes::iterator Attributes::begin() const
{
  return m_begin;
}

Attributes::iterator Attributes::end() const
{
  return m_end;
}

size_t Attributes::size() const
{
  return std::distance(begin(), end());
}

const Attribute& Attributes::at(size_t index) const
{
  return *(begin() + index);
}

/*!
 * \endclass
 */


 /*!
  * \class AttributesMap
  */

void AttributesMap::add(const void* elem, const AttributeVector& attrs)
{
  if (attrs.empty())
    return;

  Span span;
  span.first = m_attributes.size();
  span.second = span.first + attrs.size();

  m_attributes.insert(m_attributes.end(), attrs.begin(), attrs.end());
  m_map[elem] = span;
}

Attributes AttributesMap::getAttributesFor(const void* elem) const
{
  auto it = m_map.find(elem);

  if (it == m_map.end())
    return Attributes();

  Span span = it->second;
  return Attributes(m_attributes.begin() + span.first, m_attributes.begin() + span.second);
}

void AttributesMap::clear()
{
  m_map.clear();
  m_attributes.clear();
}

/*!
 * \endclass
 */

} // namespace script
