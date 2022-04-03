// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUM_H
#define LIBSCRIPT_ENUM_H

#include "libscriptdefs.h"

#include <map>

namespace script
{

class EnumImpl;

class Class;
class Engine;
class Namespace;
class Operator;
class Script;

/*!
 * \class Enum
 * \brief represents an enumeration type
 */

class LIBSCRIPT_API Enum
{
public:
  Enum() = default;
  Enum(const Enum &) = default;
  ~Enum() = default;

  explicit Enum(const std::shared_ptr<EnumImpl> & impl);

  int id() const;
  bool isNull() const;

  const std::string& name() const;

  bool isEnumClass() const;

  const std::map<std::string, int>& values() const;
  bool hasKey(const std::string & k) const;
  bool hasValue(int val) const;
  int getValue(const std::string& k, int defaultValue = -1) const;
  const std::string & getKey(int val) const;
  int addValue(const std::string& key, int value = -1);

  Operator getAssignmentOperator() const;

  Class memberOf() const;
  Namespace enclosingNamespace() const;

  Enum & operator=(const Enum &) = default;

  Engine* engine() const;
  Script script() const;

  inline const std::shared_ptr<EnumImpl> & impl() const { return d; }

private:
  std::shared_ptr<EnumImpl> d;
};

inline bool operator==(const Enum& lhs, const Enum& rhs)
{
  return lhs.impl() == rhs.impl();
}

inline bool operator!=(const Enum& lhs, const Enum& rhs)
{
  return lhs.impl() != rhs.impl();
}

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_ENUM_H
