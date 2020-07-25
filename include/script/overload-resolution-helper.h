// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OVERLOAD_RESOLUTION_HELPER_H
#define LIBSCRIPT_OVERLOAD_RESOLUTION_HELPER_H

#include "script/types.h"

namespace script
{

class Engine;
class Initialization;

template<typename T>
struct overload_resolution_helper
{
  static bool is_null(const T&) = delete;
  static Type get_type(const T&) = delete;
  static Initialization init(const Type&, const T&, Engine*) = delete;
};

} // namespace script

#endif // LIBSCRIPT_OVERLOAD_RESOLUTION_HELPER_H
