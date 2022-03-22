// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
#define LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H

#include "script/ast/node.h"

#include "script/compiler/component.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/diagnostichelper.h"

#include "script/function-blueprint.h"
#include "script/scope.h"

namespace script
{

namespace compiler
{

// @TODO: does this need to be a Component ?
class LIBSCRIPT_API DefaultArgumentProcessor : Component
{
public:

  using Component::Component;

  void generic_process(const std::vector<ast::FunctionParameter> & params, FunctionBlueprint& blueprint, const Scope & scp)
  {
    const size_t param_offset = (blueprint.parent().isClass() && !blueprint.flags_.test(FunctionSpecifier::Static)) ? 1 : 0;

    size_t first_default_index = 0;
    while (first_default_index < params.size() && params.at(first_default_index).defaultValue == nullptr)
      ++first_default_index;

    if (first_default_index == params.size())
      return;

    size_t i = params.size();
    while (i-- > first_default_index)
    {
      TranslationTarget target{ this, params.at(i).name };

      if (params.at(i).defaultValue == nullptr)
        throw CompilationFailure{ CompilerError::InvalidUseOfDefaultArgument };

      blueprint.defaultargs_.push_back(generateDefaultArgument(scp, params.at(i), blueprint.prototype_.parameter(i + param_offset)));
    }
  }

protected:
  std::shared_ptr<program::Expression> generateDefaultArgument(const Scope & scp, const ast::FunctionParameter & param, const Type & t);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_DEFAULT_ARGUMENT_PROCESSOR_H
