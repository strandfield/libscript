// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/cast.h"
#include "script/private/cast_p.h"

#include "script/name.h"

namespace script
{

CastImpl::CastImpl(const Prototype &p, Engine *e, FunctionFlags f)
  : FunctionImpl(e, f)
  , proto_(p.returnType(), p.at(0))
{

}

Name CastImpl::get_name() const
{
  return Name(SymbolKind::Cast, proto_.returnType());
}

const Prototype & CastImpl::prototype() const
{
  return this->proto_;
}

bool CastImpl::is_native() const
{
  return false;
}

std::shared_ptr<program::Statement> CastImpl::body() const
{
  return program_;
}

void CastImpl::set_body(std::shared_ptr<program::Statement> b)
{
  program_ = b;
}


Cast::Cast(const std::shared_ptr<CastImpl> & impl)
  : Function(impl)
{

}

Type Cast::sourceType() const
{
  return d->prototype().at(0);
}

Type Cast::destType() const
{
  return d->prototype().returnType();
}


std::shared_ptr<CastImpl> Cast::impl() const
{
  return std::static_pointer_cast<CastImpl>(d);
}

} // namespace script
