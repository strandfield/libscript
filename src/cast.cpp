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

SymbolKind CastImpl::get_kind() const
{
  return SymbolKind::Cast;
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

/*!
 * \class Cast
 */

Cast::Cast(const std::shared_ptr<CastImpl> & impl)
  : Function(impl)
{

}

Cast::Cast(const Function& f)
  : Function(f.isCast() ? f.impl() : nullptr)
{

}

/*!
 * \fn Type sourceType() const
 * \brief returns the conversion source type
 */
Type Cast::sourceType() const
{
  return d->prototype().at(0);
}

/*!
 * \fn Type destType() const
 * \brief returns the conversion destination type
 */
Type Cast::destType() const
{
  return d->prototype().returnType();
}

/*!
 * \endclass
 */

} // namespace script
