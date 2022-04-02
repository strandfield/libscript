// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DEFAULTARGUMENTS_MAP_H
#define LIBSCRIPT_DEFAULTARGUMENTS_MAP_H

#include "script/defaultarguments.h"

#include <unordered_map>

namespace script
{

/*!
 * \class DefaultArgumentsMap
 * \brief generic class for storing default arguments
 */
class LIBSCRIPT_API DefaultArgumentsMap
{
public:
  DefaultArgumentsMap() = default;
  DefaultArgumentsMap(const DefaultArgumentsMap&) = default;
  DefaultArgumentsMap(DefaultArgumentsMap&&) noexcept = default;
  ~DefaultArgumentsMap() = default;

  void add(const void* elem, const DefaultArgumentVector& dargs);
  DefaultArguments get(const void* elem) const;

  void clear();

  DefaultArgumentsMap& operator=(const DefaultArgumentsMap&) = default;
  DefaultArgumentsMap& operator=(DefaultArgumentsMap&&) = default;

private:
  using Span = std::pair<size_t, size_t>;
  std::unordered_map<const void*, Span> m_map;
  DefaultArgumentVector m_data;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_DEFAULTARGUMENTS_MAP_H
