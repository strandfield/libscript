// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_H
#define LIBSCRIPT_SYMBOL_H

#include "libscriptdefs.h"
#include "script/symbol-kind.h"
#include "script/types.h"

namespace script
{

class Attributes;
class Class;
class Engine;
class Function;
class Name;
class Namespace;
class Script;
class SymbolImpl;

/*!
 * \class Symbol
 * \brief provides storage for any symbol (class, namespace, etc...)
 */

class LIBSCRIPT_API Symbol
{
public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  ~Symbol() = default;

  explicit Symbol(const script::Class & c);
  explicit Symbol(const Namespace & n);
  Symbol(const Function& f);

  explicit Symbol(const std::shared_ptr<SymbolImpl> & impl);

  typedef SymbolKind Kind;

  inline bool isNull() const { return d == nullptr; }

  Engine* engine() const;

  Kind kind() const;

  bool isClass() const;
  script::Class toClass() const;

  bool isNamespace() const;
  Namespace toNamespace() const;

  bool isFunction() const;
  Function toFunction() const;

  Name name() const;
  Symbol parent() const;
  Script script() const;

  Attributes attributes() const;

  Symbol & operator=(const Symbol &) = default;

  inline const std::shared_ptr<SymbolImpl> & impl() const { return d; }

private:
  std::shared_ptr<SymbolImpl> d;
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_SYMBOL_H
