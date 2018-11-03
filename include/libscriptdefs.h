// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_GLOBAL_DEFS_H
#define LIBSCRIPT_GLOBAL_DEFS_H

#include <string>
#include <cassert>
#include <memory>

#if defined(WIN32) || defined(_WIN32)
#if defined(LIBSCRIPT_COMPILE_LIBRARY)
#  define LIBSCRIPT_API __declspec(dllexport)
#else
#  define LIBSCRIPT_API __declspec(dllimport)
#endif
#else
#define LIBSCRIPT_API
#endif

#if defined(LIBSCRIPT_HAS_CONFIG)
#include LIBSCRIPT_CONFIG_HEADER
#endif //defined(LIBSCRIPT_HAS_CONFIG)

namespace script
{
using reference_counter_type = int;

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef signed long long int64;
typedef unsigned long long uint64;

enum class ParameterPolicy {
  Copy,
  Move,
  Take
};

} // namespace script

#endif // LIBSCRIPT_GLOBAL_DEFS_H
