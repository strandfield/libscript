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

void DefaultArgumentProcessor::process(const std::vector<ast::FunctionParameter> & params, Function & f, const Scope & scp)
{
  const int param_offset = f.hasImplicitObject() ? 1 : 0;

  size_t i = 0;
  while (i < params.size() && params.at(i).defaultValue == nullptr)
    ++i;

  if (i == params.size())
    return;

  while (i < params.size())
  {
    if (params.at(i).defaultValue == nullptr)
      throw InvalidUseOfDefaultArgument{ dpos(params.at(i).name) };

    f.addDefaultArgument(generateDefaultArgument(scp, params.at(i), f.parameter(i + param_offset)));

    ++i;
  }
}

std::shared_ptr<program::Expression> DefaultArgumentProcessor::generateDefaultArgument(const Scope & scp, const ast::FunctionParameter & param, const Type & t)
{
  ExpressionCompiler ec{ scp };
  auto expr = ec.generateExpression(param.defaultValue);

  ConversionSequence conv = ConversionSequence::compute(expr, t, scp.engine());
  if (conv == ConversionSequence::NotConvertible())
    throw NotImplementedError{ "FunctionCompiler::generateDefaultArgument() : failed to convert default value" };

  return ConversionProcessor::convert(scp.engine(), expr, t, conv);
}

} // namespace compiler

} // namespace script

