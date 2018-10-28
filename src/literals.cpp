// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/literals.h"
#include "script/private/literals_p.h"

#include "script/name.h"

namespace script
{

LiteralOperatorImpl::LiteralOperatorImpl(std::string && suffix, const Prototype & proto, Engine *engine, FunctionFlags flags)
  : FunctionImpl(engine, flags)
  , suffix(std::move(suffix))
  , proto_(proto)
{

}

Name LiteralOperatorImpl::get_name() const
{
  return Name{ Name::LiteralOperatorTag{}, suffix };
}

const Prototype & LiteralOperatorImpl::prototype() const
{
  return this->proto_;
}


LiteralOperator::LiteralOperator(const std::shared_ptr<LiteralOperatorImpl> & impl)
  : Function(impl)
{

}

Type LiteralOperator::input() const
{
  return d->prototype().at(0);
}

Type LiteralOperator::output() const
{
  return d->prototype().returnType();
}

const std::string & LiteralOperator::suffix() const
{
  return impl()->suffix;
}

LiteralOperatorImpl * LiteralOperator::impl() const
{
  return dynamic_cast<LiteralOperatorImpl*>(d.get());
}

} // namespace script
