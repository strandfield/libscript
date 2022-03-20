// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOLKIND_H
#define LIBSCRIPT_SYMBOLKIND_H

#include "libscriptdefs.h"

namespace script
{

/*!
 * \enum SymbolKind
 * \brief list the different kinds of symbol
 */
enum class SymbolKind
{
  NotASymbol = 0,
  Namespace,
  Class,
  Function,
  Constructor,
  Destructor,
  Cast,
  Operator,
  LiteralOperator,
  Template,
};

} // namespace script

#endif // LIBSCRIPT_SYMBOLKIND_H
