// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/typedefs.h"

#include "script/class.h"
#include "script/namespace.h"

#include "script/private/class_p.h"
#include "script/private/namespace_p.h"

namespace script
{

void TypedefBuilder::create()
{
  if (this->symbol_.isClass())
    this->symbol_.toClass().impl()->typedefs.push_back(Typedef{ std::move(this->name_), this->type_ });
  else if (this->symbol_.isNamespace())
    this->symbol_.toNamespace().impl()->typedefs.push_back(Typedef{ std::move(this->name_), this->type_ });

  /// TODO ? Should we throw on error ?
}

} // namespace script