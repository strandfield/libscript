// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/defaultargumentprocessor.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/diagnostichelper.h"
#include "script/compiler/expressioncompiler.h"

#include "script/ast/node.h"

#include "script/engine.h"
#include "script/function-blueprint.h"

namespace script
{

namespace compiler
{

static std::shared_ptr<program::Expression> generateDefaultArgument(ExpressionCompiler& ec, const ast::FunctionParameter & param, const Type & t)
{
  auto expr = ec.generateExpression(param.defaultValue);

  Conversion conv = Conversion::compute(expr, t, ec.engine());
  if (conv.isInvalid())
    throw NotImplemented{ "FunctionCompiler::generateDefaultArgument() : failed to convert default value" };

  return ConversionProcessor::convert(ec.engine(), expr, conv);
}

/*!
 * \fn DefaultArgumentVector process_default_arguments(ExpressionCompiler& ec, const std::vector<ast::FunctionParameter>& params, const Function& f)
 * \brief computes the default arguments of a function
 */
DefaultArgumentVector process_default_arguments(ExpressionCompiler& ec, const std::vector<ast::FunctionParameter>& params, const Function& f)
{
  DefaultArgumentVector result;

  const size_t param_offset = f.isNonStaticMemberFunction() ? 1 : 0;

  size_t first_default_index = 0;
  while (first_default_index < params.size() && params.at(first_default_index).defaultValue == nullptr)
    ++first_default_index;

  if (first_default_index == params.size())
    return result;

  size_t i = params.size();
  while (i-- > first_default_index)
  {
    if (params.at(i).defaultValue == nullptr)
      throw CompilationFailure{ CompilerError::InvalidUseOfDefaultArgument };

    result.push_back(generateDefaultArgument(ec, params.at(i), f.parameter(i + param_offset)));
  }

  return result;
}

} // namespace compiler

} // namespace script

