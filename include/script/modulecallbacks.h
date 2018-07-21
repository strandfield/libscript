// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_CALLBACKS_H
#define LIBSCRIPT_MODULE_CALLBACKS_H

#include "libscriptdefs.h"

namespace script
{

class Module;

typedef void(*ModuleLoadFunction)(Module);
typedef void(*ModuleCleanupFunction)(Module);

} // namespace script

#endif // LIBSCRIPT_MODULE_CALLBACKS_H
