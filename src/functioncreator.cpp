// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/functioncreator.h"

#include "script/private/cast_p.h"
#include "script/private/function_p.h"
#include "script/private/literals_p.h"
#include "script/private/operator_p.h"
#include "script/private/script_p.h"

#include "script/program/statements.h"

#include "script/ast/node.h"

#include "script/class.h"
#include "script/namespace.h"
#include "script/script.h"
#include "script/symbol.h"

namespace script
{

template<typename FT>
static void generic_fill(const std::shared_ptr<FT>& impl, const FunctionBlueprint& opts)
{
  impl->program_ = opts.body();
  impl->data = opts.data();
  impl->enclosing_symbol = opts.parent().impl();
}

inline static void set_default_args(Function& fun, const std::vector<DefaultArgument>& dargs)
{
  Script s = fun.script();

  if (!s.isNull())
    s.impl()->defaultarguments.add(fun.impl().get(), dargs);
}

/*!
 * \class FunctionBuilder
 */

FunctionCreator::~FunctionCreator()
{

}

/*!
 * \fn Function create(FunctionBlueprint& blueprint, const std::shared_ptr<ast::FunctionDeclaration>& fdecl, std::vector<Attribute>& attrs)
 * \brief creates a function from a blueprint
 * \param the blueprint
 * \param the function declaration
 * \param the attributes
 * 
 * This function creates a function from the blueprint.
 * The function should not be added to its future parent.
 * 
 * The default implementation does not use the attributes and supports a null function declaration.
 */
Function FunctionCreator::create(FunctionBlueprint& blueprint, const std::shared_ptr<ast::FunctionDecl>& fdecl, std::vector<Attribute>& /* attrs */)
{
  if (fdecl && (fdecl->body || fdecl->defaultKeyword.isValid()))
  {
    if (!blueprint.body_)
      blueprint.body_ = compile_later();
  }

  if (blueprint.name_.kind() == SymbolKind::Function)
  {
    auto impl = std::make_shared<RegularFunctionImpl>(blueprint.name_.string(), std::move(blueprint.prototype_), blueprint.engine(), blueprint.flags_);
    generic_fill(impl, blueprint);
    Function ret{ impl };
    set_default_args(ret, blueprint.defaultargs_);
    return ret;
  }
  else if (blueprint.name_.kind() == SymbolKind::Operator)
  {
    OperatorName operation = blueprint.name_.operatorName();

    if (operation == OperatorName::FunctionCallOperator)
    {
      auto impl = std::make_shared<FunctionCallOperatorImpl>(OperatorName::FunctionCallOperator, std::move(blueprint.prototype_), blueprint.engine(), blueprint.flags_);
      generic_fill(impl, blueprint);
      Operator ret{ impl };
      set_default_args(ret, blueprint.defaultargs_);
      return ret;
    }
    else
    {
      std::shared_ptr<OperatorImpl> impl;

      if (Operator::isBinary(operation))
        impl = std::make_shared<BinaryOperatorImpl>(operation, blueprint.prototype_, blueprint.engine(), blueprint.flags_);
      else
        impl = std::make_shared<UnaryOperatorImpl>(operation, blueprint.prototype_, blueprint.engine(), blueprint.flags_);

      generic_fill(impl, blueprint);
      return Operator{ impl };
    }
  }
  else if (blueprint.name_.kind() == SymbolKind::Cast)
  {
    auto impl = std::make_shared<CastImpl>(blueprint.prototype_, blueprint.engine(), blueprint.flags_);
    generic_fill(impl, blueprint);
    Function ret{ impl };
    return ret;
  }
  else if (blueprint.name_.kind() == SymbolKind::Constructor)
  {
    auto impl = std::make_shared<ConstructorImpl>(blueprint.prototype_, blueprint.engine(), blueprint.flags_);
    generic_fill(impl, blueprint);
    Function ret{ impl };
    set_default_args(ret, blueprint.defaultargs_);
    return ret;
  }
  else if (blueprint.name_.kind() == SymbolKind::Destructor)
  {
    auto impl = std::make_shared<DestructorImpl>(blueprint.prototype_, blueprint.engine(), blueprint.flags_);
    generic_fill(impl, blueprint);
    Function ret{ impl };
    return ret;
  }
  else if (blueprint.name_.kind() == SymbolKind::LiteralOperator)
  {
    auto impl = std::make_shared<LiteralOperatorImpl>(blueprint.name_.string(), blueprint.prototype_, blueprint.engine(), blueprint.flags_);
    generic_fill(impl, blueprint);
    Function ret{ impl };
    return ret;
  }
  else
  {
    assert(false);
    return {};
  }
}

std::shared_ptr<program::Statement> FunctionCreator::compile_later()
{
  static const std::shared_ptr<program::Statement> static_instance = std::make_shared<program::CompoundStatement>();
  return static_instance;
}

/*!
 * \endclass
 */

} // namespace script
