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
class Statement;
} // namespace program

class Class;
class Namespace;

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
  std::shared_ptr<program::Statement> body_;
  FunctionFlags flags_;
  std::shared_ptr<UserData> data_;

public:
  FunctionBlueprint(Symbol s, SymbolKind k, std::string name);
  FunctionBlueprint(Symbol s, SymbolKind k, Type t);
  FunctionBlueprint(Symbol s, SymbolKind k, OperatorName n);

  explicit FunctionBlueprint(Symbol s);

  static FunctionBlueprint Fun(Class c, std::string name);
  static FunctionBlueprint Fun(Namespace ns, std::string name);
  static FunctionBlueprint Constructor(Class c);
  static FunctionBlueprint Destructor(Class c);
  static FunctionBlueprint Op(Class c, OperatorName op);
  static FunctionBlueprint Op(Namespace ns, OperatorName op);
  static FunctionBlueprint LiteralOp(Namespace ns, std::string suffix);
  static FunctionBlueprint Cast(Class c);
  
  Engine* engine() const;
  Symbol parent() const;
  const Name& name() const;
  const DynamicPrototype& prototype() const;
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

inline const Name& FunctionBlueprint::name() const
{
  return name_;
}

inline const DynamicPrototype& FunctionBlueprint::prototype() const
{
  return prototype_;
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