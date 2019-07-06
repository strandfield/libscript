// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/typesystemtransaction.h"

#include "script/private/typesystem_p.h"

namespace script
{

/*!
 * \class TypeSystemTransaction
 * \brief Provides safe way to modify the type-system.
 */

/*!
 * \fn TypeSystemTransaction(TypeSystem* ts = nullptr)
 * \brief Constructs a transaction object.
 *
 * If \a ts is not null, starts a transaction.
 */
TypeSystemTransaction::TypeSystemTransaction(TypeSystem* ts)
  : m_target(ts)
{
  if (m_target)
  {
    start();
  }
}

/*!
 * \fn ~TypeSystemTransaction()
 * \brief Destroys the transaction object
 *
 * If there are uncaught exceptions and \m isActive() is true, 
 * this calls \m rollback(), otherwise this calls \m commit().
 */
TypeSystemTransaction::~TypeSystemTransaction()
{
  if (isActive())
  {
    if (std::uncaught_exception())
    {
      rollback();
    }
    else
    {
      commit();
    }
  }
}

/*!
 * \fn bool isActive() const
 * \brief Returns whether the transaction is active.
 */
bool TypeSystemTransaction::isActive() const
{
  return TypeSystemListener::typeSystem() != nullptr;
}

/*!
 * \fn void start()
 * \brief Starts a transaction on the typesystem passed through the constructor.
 */
void TypeSystemTransaction::start()
{
  m_target->addListener(this);
}

/*!
 * \fn void start(TypeSystem* ts)
 * \brief Starts a transaction on the given typesystem.
 */
void TypeSystemTransaction::start(TypeSystem* ts)
{
  m_target = ts;
  start();
}

/*!
 * \fn void commit()
 * \brief Ends the currently active transaction and validate all changes.
 */
void TypeSystemTransaction::commit()
{
  TypeSystemListener::typeSystem()->removeListener(this);
  m_types.clear();
}

/*!
 * \fn void commit()
 * \brief Reverse all changes made since the beginning of the transaction.
 */
void TypeSystemTransaction::rollback()
{
  TypeSystemImpl* ts = TypeSystemListener::typeSystem()->impl();
  auto types = std::move(m_types);

  for (Type t : types)
  {
    ts->destroy(t);
  }

  TypeSystemListener::typeSystem()->removeListener(this);
  m_types.clear();
}

void TypeSystemTransaction::created(const Type& t)
{
  m_types.push_back(t);
}

void TypeSystemTransaction::destroyed(const Type& t)
{
  auto it = std::find(m_types.begin(), m_types.end(), t);

  if (it != m_types.end())
  {
    m_types.erase(it);
  }
}

} // namespace script
