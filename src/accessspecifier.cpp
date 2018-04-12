// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/accessspecifier.h"

#include "script/class.h"

namespace script
{

static inline bool has_friendship(const Function & f, const Class & cla)
{
  {
    const auto & friends = cla.friends(Function{});
    auto it = std::find(friends.begin(), friends.end(), f);
    if (it != friends.end())
      return true;
  }

  {
    const auto & friends = cla.friends(Class{});
    auto it = std::find(friends.begin(), friends.end(), f.memberOf());
    if (it != friends.end())
      return true;
  }

  return false;
}

static inline bool check_protected(const Function & f, const Class & cla)
{
  if (f.isMemberFunction() && f.memberOf().inherits(cla))
    return true;

  return has_friendship(f, cla);
}

static inline bool check_private(const Function & f, const Class & cla)
{
  if (f.isMemberFunction() && f.memberOf() == cla)
    return true;

  return has_friendship(f, cla);
}


bool Accessibility::check(const Function & f, const Class & cla, AccessSpecifier aspec)
{
  switch (aspec)
  {
  case script::AccessSpecifier::Public:
    return true;
  case script::AccessSpecifier::Protected:
    return !f.isNull() && check_protected(f, cla);
  case script::AccessSpecifier::Private:
    return !f.isNull() && check_private(f, cla);
  }

  assert(false);
  throw std::runtime_error("Accessibility::check() : implementation error");
}

bool Accessibility::check(const Function & caller, const Function & member)
{
  return Accessibility::check(caller, member.memberOf(), member.accessibility());
}

} // namespace script
