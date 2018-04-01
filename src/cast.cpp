// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/cast.h"
#include "cast_p.h"

namespace script
{

CastImpl::CastImpl(const Prototype &p, Engine *e, uint8 f)
  : FunctionImpl(p, e, f)
{

}

std::string CastImpl::name() const
{
  throw std::runtime_error{ "Not implemented" };
}

Cast::Cast(const std::shared_ptr<CastImpl> & impl)
  : Function(impl)
{

}

Type Cast::sourceType() const
{
  return d->prototype.argv(0);
}

Type Cast::destType() const
{
  return d->prototype.returnType();
}


CastImpl * Cast::implementation() const
{
  return dynamic_cast<CastImpl*>(d.get());
}

} // namespace script
