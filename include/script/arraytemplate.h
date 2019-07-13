// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ARRAY_TEMPLATE_H
#define LIBSCRIPT_ARRAY_TEMPLATE_H

#include "script/classtemplatenativebackend.h"

namespace script
{

class LIBSCRIPT_API ArrayTemplate : public ClassTemplateNativeBackend
{
  Class instantiate(ClassTemplateInstanceBuilder& builder) override;
};

} // namespace script

#endif // LIBSCRIPT_ARRAY_TEMPLATE_H
