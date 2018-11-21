// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_STRING_H
#define LIBSCRIPT_STRING_H

#include "libscriptdefs.h"

#if defined(LIBSCRIPT_HAS_CONFIG)
#include "config/libscript/string.h"
#else 

#include <string>

namespace script
{
using String = std::string;
} // namespace script

#endif // defined(LIBSCRIPT_HAS_CONFIG)

#endif // LIBSCRIPT_STRING_H
