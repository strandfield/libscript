// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/literals.h"
#include "script/private/literals_p.h"

#include "script/name.h"

namespace script
{

LiteralOperatorImpl::LiteralOperatorImpl(std::string suffix, const Prototype & proto, Engine *engine, FunctionFlags flags)
  : FunctionImpl(engine, flags)
  , suffix(std::move(suffix))
  , proto_(proto)
{

}

SymbolKind LiteralOperatorImpl::get_kind() const
{
  return SymbolKind::LiteralOperator;
}

const std::string& LiteralOperatorImpl::literal_operator_suffix() const
{
  return suffix;
}

Name LiteralOperatorImpl::get_name() const
{
  return Name(SymbolKind::LiteralOperator, suffix);
}

const Prototype & LiteralOperatorImpl::prototype() const
{
  return this->proto_;
}

bool LiteralOperatorImpl::is_native() const
{
  return false;
}

std::shared_ptr<program::Statement> LiteralOperatorImpl::body() const
{
  return program_;
}

void LiteralOperatorImpl::set_body(std::shared_ptr<program::Statement> b)
{
  program_ = b;
}

/*!
 * \class Operator
 */

LiteralOperator::LiteralOperator(const std::shared_ptr<LiteralOperatorImpl> & impl)
  : Function(impl)
{

}

LiteralOperator::LiteralOperator(const Function& f)
  : Function(f.isLiteralOperator() ? f.impl() : nullptr)
{

}

/*!
 * \fn Type input() const
 * \brief returns the input type of this literal operator
 */
Type LiteralOperator::input() const
{
  return d->prototype().at(0);
}

/*!
 * \fn Type output() const
 * \brief returns the output type of this literal operator
 */
Type LiteralOperator::output() const
{
  return d->prototype().returnType();
}

/*!
 * \fn const std::string& suffix() const
 * \brief returns the suffix of this literal operator
 */
const std::string& LiteralOperator::suffix() const
{
  return d->literal_operator_suffix();
}

/*!
 * \endclass
 */

} // namespace script
