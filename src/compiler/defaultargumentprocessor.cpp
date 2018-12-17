// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/defaultargumentprocessor.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/diagnostichelper.h"
#include "script/compiler/expressioncompiler.h"

#include "script/ast/node.h"

namespace script
{

namespace compiler
{

std::shared_ptr<program::Expression> DefaultArgumentProcessor::generateDefaultArgument(const Scope & scp, const ast::FunctionParameter & param, const Type & t)
{
  ExpressionCompiler ec{ scp };
  auto expr = ec.generateExpression(param.defaultValue);

  Conversion conv = Conversion::compute(expr, t, scp.engine());
  if (conv.isInvalid())
    throw NotImplemented{ "FunctionCompiler::generateDefaultArgument() : failed to convert default value" };

  return ConversionProcessor::convert(scp.engine(), expr, conv);
}

} // namespace compiler

} // namespace script

