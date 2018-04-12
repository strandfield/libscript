// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ACCESS_SPECIFIER_H
#define LIBSCRIPT_ACCESS_SPECIFIER_H

#include "libscriptdefs.h"

namespace script
{

class Class;
class Function;

enum class AccessSpecifier {
  Public = 0,
  Protected,
  Private,
};

struct Accessibility
{
  static bool check(const Function & f, const Class & cla, AccessSpecifier aspec);
  static bool check(const Function & caller, const Function & member);
};

} // namespace script

#endif // LIBSCRIPT_ACCESS_SPECIFIER_H
