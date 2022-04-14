// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
#define LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H

#include "script/ast/node.h"

#include "script/defaultarguments.h"

namespace script
{

class Function;

namespace compiler
{

class ExpressionCompiler;

LIBSCRIPT_API DefaultArgumentVector process_default_arguments(ExpressionCompiler& ec, const std::vector<ast::FunctionParameter>& params, const Function& f);

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
