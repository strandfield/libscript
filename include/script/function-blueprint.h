// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_BLUEPRINT_H
#define LIBSCRIPT_FUNCTION_BLUEPRINT_H

#include "script/accessspecifier.h"
#include "script/callbacks.h"
#include "script/functionflags.h"
#include "script/name.h"
#include "script/prototypes.h"
#include "script/symbol.h"
#include "script/userdata.h"

#include <memory>
#include <vector>

namespace script
{

namespace program
{
class Expression;
class Statement;
} // namespace program

/*!
 * \class FunctionBlueprint
 * \brief describe everything needed to build a function
 */
class FunctionBlueprint
{
public:
  Symbol parent_;
  Name name_;
  DynamicPrototype prototype_;
  std::vector<std::shared_ptr<program::Expression>> defaultargs_;
  std::shared_ptr<program::Statement> body_;
  FunctionFlags flags_;
  std::shared_ptr<UserData> data_;

public:
  FunctionBlueprint(const Symbol& s)
    : parent_(s) 
  {
  }
  
  Engine* engine() const;
  Symbol parent() const;
  const DynamicPrototype& prototype() const;
  const std::vector<std::shared_ptr<program::Expression>>& defaultargs() const;
  const std::shared_ptr<program::Statement>& body() const;
  FunctionFlags flags() const;
  const std::shared_ptr<UserData>& data() const;

  // @TODO: find a better place for this function
  void setStatic();
};

inline Engine* FunctionBlueprint::engine() const
{
  return parent_.engine();
}

inline Symbol FunctionBlueprint::parent() const
{
  return parent_;
}

inline const DynamicPrototype& FunctionBlueprint::prototype() const
{
  return prototype_;
}

inline const std::vector<std::shared_ptr<program::Expression>>& FunctionBlueprint::defaultargs() const
{
  return defaultargs_;
}

inline const std::shared_ptr<program::Statement>& FunctionBlueprint::body() const
{
  return body_;
}

inline FunctionFlags FunctionBlueprint::flags() const
{
  return flags_;
}

inline const std::shared_ptr<UserData>& FunctionBlueprint::data() const
{
  return data_;
}

inline void FunctionBlueprint::setStatic()
{
  flags_.set(FunctionSpecifier::Static);

  if (prototype_.count() == 0 || !prototype_.at(0).testFlag(Type::ThisFlag))
    return;

  for (int i(0); i < prototype_.count() - 1; ++i)
    prototype_.setParameter(i, prototype_.at(i + 1));
  prototype_.pop();
}

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_FUNCTION_BLUEPRINT_H