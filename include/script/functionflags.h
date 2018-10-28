// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTIONFLAGS_H
#define LIBSCRIPT_FUNCTIONFLAGS_H

#include "script/accessspecifier.h"

namespace script
{

enum class FunctionCategory {
  StandardFunction = 0,
  Constructor = 1,
  Destructor = 2,
  OperatorFunction = 3,
  CastFunction = 4,
  Root = 5,
  LiteralOperatorFunction = 6,
};

enum class FunctionSpecifier {
  None = 0,
  Static = 1,
  Explicit = 2,
  Virtual = 4,
  Pure = 8,
  ConstExpr = 16,
  Default = 32,
  Delete = 64,
};

enum ImplementationMethod {
  NativeFunction = 0,
  InterpretedFunction = 1
};

class LIBSCRIPT_API FunctionFlags
{
public:
  FunctionFlags()
    : d(0) {}

  FunctionFlags(const FunctionFlags &) = default;

  explicit FunctionFlags(FunctionSpecifier val);

  bool test(FunctionSpecifier fs) const;
  void set(FunctionSpecifier fs);

  bool test(ImplementationMethod im) const;
  void set(ImplementationMethod im);

  AccessSpecifier getAccess() const;
  void set(AccessSpecifier as);

  FunctionFlags & operator=(const FunctionFlags &) = default;

private:
  uint16_t d;
};

} // namespace script

#endif // LIBSCRIPT_FUNCTIONFLAGS_H
