// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
#define LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H

#include "script/ast/forwards.h"
#include "script/function.h" // ideally this should be removed
#include "script/functionbuilder.h"
#include "script/scope.h"

namespace script
{

namespace compiler
{

class LIBSCRIPT_API DefaultArgumentProcessor
{
public:

  void process(const std::vector<ast::FunctionParameter> & params, FunctionBuilder & builder, const Scope & scp);

  template<typename Builder>
  void generic_process(const std::vector<ast::FunctionParameter> & params, Builder & builder, const Scope & scp)
  {
    const int param_offset = (builder.symbol.isClass() && !builder.isStatic()) ? 1 : 0;

    size_t first_default_index = 0;
    while (first_default_index < params.size() && params.at(first_default_index).defaultValue == nullptr)
      ++first_default_index;

    if (first_default_index == params.size())
      return;

    size_t i = params.size();
    while (i-- > first_default_index)
    {
      if (params.at(i).defaultValue == nullptr)
        throw InvalidUseOfDefaultArgument{ dpos(params.at(i).name) };

      builder.addDefaultArgument(generateDefaultArgument(scp, params.at(i), builder.proto_.parameter(i + param_offset)));
    }
  }

protected:
  std::shared_ptr<program::Expression> generateDefaultArgument(const Scope & scp, const ast::FunctionParameter & param, const Type & t);


};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
