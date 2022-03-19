// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ATTRIBUTES_MAP_H
#define LIBSCRIPT_ATTRIBUTES_MAP_H

#include "script/attributes.h"

#include <unordered_map>

namespace script
{

/*!
 * \class AttributesMap
 * \brief generic class for storing attributes
 */
class LIBSCRIPT_API AttributesMap
{
public:
  AttributesMap() = default;
  AttributesMap(const AttributesMap&) = default;
  AttributesMap(AttributesMap&&) noexcept = default;
  ~AttributesMap() = default;

  void add(const void* elem, const AttributeVector& attrs);
  Attributes getAttributesFor(const void* elem) const;

  void clear();

  AttributesMap& operator=(const AttributesMap&) = default;
  AttributesMap& operator=(AttributesMap&&) = default;

private:
  using Span = std::pair<size_t, size_t>;
  std::unordered_map<const void*, Span> m_map;
  AttributeVector m_attributes;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_ATTRIBUTES_MAP_H
