// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_VALUE_CONSTRUCTOR_H
#define LIBSCRIPT_COMPILER_VALUE_CONSTRUCTOR_H

#include "script/conversions.h"
#include "script/diagnosticmessage.h"

#include "script/ast/forwards.h"

#include "script/program/expression.h"

namespace script
{

class Initialization;

namespace compiler
{

class ExpressionCompiler;

class ValueConstructor
{
public:

  static Value fundamental(Engine *e, const Type & t);

  static std::shared_ptr<program::Expression> fundamental(Engine *e, const Type & t, bool copy);

  static std::shared_ptr<program::Expression> construct(Engine *e, const Type & t, std::nullptr_t, diagnostic::pos_t dp);

  static std::shared_ptr<program::Expression> brace_construct(Engine *e, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp);
  static std::shared_ptr<program::Expression> construct(Engine *e, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp);

  static std::shared_ptr<program::Expression> construct(ExpressionCompiler & ec, const Type & t, const std::shared_ptr<ast::ConstructorInitialization> & init);
  static std::shared_ptr<program::Expression> construct(ExpressionCompiler & ec, const Type & t, const std::shared_ptr<ast::BraceInitialization> & init);

  static std::shared_ptr<program::Expression> construct(Engine *e, const Type & t, std::shared_ptr<program::Expression> & arg, const Initialization & init);
  static std::shared_ptr<program::Expression> construct(Engine *e, const Type & t, std::vector<std::shared_ptr<program::Expression>> && args, const Initialization & init);

  /// TODO: maybe rename to initParameters()
  static void prepare(Engine *e, std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<Initialization> & inits);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_VALUE_CONSTRUCTOR_H
