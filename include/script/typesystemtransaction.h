// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPESYSTEMTRANSACTION_H
#define LIBSCRIPT_TYPESYSTEMTRANSACTION_H

#include <vector>

#include "script/typesystem.h"
#include "script/typesystemlistener.h"

namespace script
{

class LIBSCRIPT_API TypeSystemTransaction : public TypeSystemListener
{
public:
  explicit TypeSystemTransaction(TypeSystem* ts = nullptr);
  TypeSystemTransaction(const TypeSystemTransaction&) = delete;
  ~TypeSystemTransaction();

  bool isActive() const;

  void start();
  void start(TypeSystem* ts);

  void commit();
  void rollback();

protected:
  void created(const Type& t) override;
  void destroyed(const Type& t) override;

private:
  TypeSystem* m_target;
  std::vector<Type> m_types;
};

} // namespace script

#endif // !LIBSCRIPT_TYPESYSTEMTRANSACTION_H
