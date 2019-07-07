// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_NATIVE_BACKEND_H
#define LIBSCRIPT_TEMPLATE_NATIVE_BACKEND_H

#include "libscriptdefs.h"

namespace script
{

class LIBSCRIPT_API TemplateNativeBackend
{
public:
  TemplateNativeBackend() = default;
  TemplateNativeBackend(const TemplateNativeBackend& ) = delete;
  virtual ~TemplateNativeBackend() = default;

};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_NATIVE_BACKEND_H
