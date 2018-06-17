// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
#define LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H

#include "script/ast/forwards.h"
#include "script/function.h"
#include "script/scope.h"

namespace script
{

namespace compiler
{

class LIBSCRIPT_API DefaultArgumentProcessor
{
public:

  void process(const std::vector<ast::FunctionParameter> & params, Function & f, const Scope & scp);

protected:
  std::shared_ptr<program::Expression> generateDefaultArgument(const Scope & scp, const ast::FunctionParameter & param, const Type & t);


};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
