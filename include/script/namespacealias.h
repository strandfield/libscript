// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAMESPACE_ALIAS_H
#define LIBSCRIPT_NAMESPACE_ALIAS_H

#include "libscriptdefs.h"

#include <string>
#include <vector>

namespace script
{

class LIBSCRIPT_API NamespaceAlias
{
public:
  NamespaceAlias() = default;
  NamespaceAlias(const NamespaceAlias &) = default;
  ~NamespaceAlias() = default;

  NamespaceAlias(const std::string & name, const std::vector<std::string> & nn)
    : mName(name)
    , mNestedNames(nn)
  {

  }

  inline bool isValid() const { return !mName.empty() && !mNestedNames.empty(); }

  inline const std::string & name() const { return mName; }
  inline const std::vector<std::string> & nested() const { return mNestedNames; }
  
  NamespaceAlias & operator=(const NamespaceAlias &) = default;

private:
  std::string mName;
  std::vector<std::string> mNestedNames;
};

} // namespace script

#endif // LIBSCRIPT_NAMESPACE_ALIAS_H
