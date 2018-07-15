// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CALLBACKS_H
#define LIBSCRIPT_CALLBACKS_H

#include "libscriptdefs.h"

namespace script
{

namespace interpreter { class FunctionCall; }
typedef interpreter::FunctionCall FunctionCall;
class Value;

typedef Value(*NativeFunctionSignature) (FunctionCall *);

} // namespace script

#endif // LIBSCRIPT_CALLBACKS_H
