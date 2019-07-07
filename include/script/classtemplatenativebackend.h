// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_TEMPLATE_NATIVE_BACKEND_H
#define LIBSCRIPT_CLASS_TEMPLATE_NATIVE_BACKEND_H

#include "script/templatenativebackend.h"

namespace script
{

class Class;
class ClassTemplateInstanceBuilder;

class LIBSCRIPT_API ClassTemplateNativeBackend : public TemplateNativeBackend
{
public:
  ClassTemplateNativeBackend() = default;
  ~ClassTemplateNativeBackend() = default;

  virtual Class instantiate(ClassTemplateInstanceBuilder& builder) = 0;
};

} // namespace script

#endif // !LIBSCRIPT_CLASS_TEMPLATE_NATIVE_BACKEND_H
