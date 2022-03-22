// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_CREATOR_H
#define LIBSCRIPT_FUNCTION_CREATOR_H

#include "script/attributes.h"
#include "script/function.h"
#include "script/function-blueprint.h"

#include <memory>
#include <vector>

namespace script
{

namespace ast
{
class FunctionDecl;
} // namespace ast

/*!
 * \class FunctionCreator
 * \brief The FunctionCreator class creates function objects
 */

class LIBSCRIPT_API FunctionCreator
{
public:
  FunctionCreator() = default;
  virtual ~FunctionCreator();

  virtual Function create(FunctionBlueprint& blueprint, const std::shared_ptr<ast::FunctionDecl>& fdecl, std::vector<Attribute>& attrs);

  static std::shared_ptr<program::Statement> compile_later();
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_FUNCTION_CREATOR_H