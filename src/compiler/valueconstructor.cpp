// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/valueconstructor.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/expressioncompiler.h"

#include "script/class.h"
#include "script/engine.h"
#include "script/initialization.h"
#include "script/overloadresolution.h"
#include "script/typesystem.h"

namespace script
{

namespace compiler
{

Value ValueConstructor::fundamental(Engine *e, const Type & t)
{
  assert(t.isFundamentalType());
  return e->construct(t.baseType(), {});
}

std::shared_ptr<program::Expression> ValueConstructor::fundamental(Engine *e, const Type & t, bool copy)
{
  Value val = fundamental(e, t);
  e->manage(val);

  auto lit = program::Literal::New(val);
  if (copy)
    return program::Copy::New(t, lit);
  return lit;
}

std::shared_ptr<program::Expression> ValueConstructor::construct(Engine *e, const Type & type, std::nullptr_t)
{
  if (type.isReference() || type.isRefRef())
    throw CompilationFailure{ CompilerError::ReferencesMustBeInitialized };

  if (type.isFundamentalType())
    return ValueConstructor::fundamental(e, type, true);
  else if (type.isEnumType())
    throw CompilationFailure{ CompilerError::EnumerationsCannotBeDefaultConstructed };
  else if (type.isFunctionType())
    throw CompilationFailure{ CompilerError::FunctionVariablesMustBeInitialized };
  else if (type.isObjectType())
  {
    Function default_ctor = e->typeSystem()->getClass(type).defaultConstructor();
    if (default_ctor.isNull())
      throw CompilationFailure{ CompilerError::VariableCannotBeDefaultConstructed };
    else if (default_ctor.isDeleted())
      throw CompilationFailure{ CompilerError::ClassHasDeletedDefaultCtor, errors::VariableType{type} };

    return program::ConstructorCall::New(default_ctor, {});
  }

  throw NotImplemented{ "ValueConstructor::construct() : cannot default construct value" };
}

std::shared_ptr<program::Expression> ValueConstructor::brace_construct(Engine *e, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args)
{
  if (args.size() == 0)
    return construct(e, type, nullptr);

  if (!type.isObjectType() && args.size() != 1)
    throw CompilationFailure{ CompilerError::TooManyArgumentInInitialization };

  if ((type.isReference() || type.isRefRef()) && args.size() != 1)
    throw CompilationFailure{ CompilerError::TooManyArgumentInReferenceInitialization };

  if (type.isFundamentalType() || type.isEnumType() || type.isFunctionType())
  {
    Conversion conv = Conversion::compute(args.front(), type, e);

    if (conv == Conversion::NotConvertible())
      throw CompilationFailure{ CompilerError::CouldNotConvert, errors::ConversionFailure{args.front()->type(), type} };

    if (conv.isNarrowing())
      throw CompilationFailure{ CompilerError::NarrowingConversionInBraceInitialization, errors::NarrowingConversion{args.front()->type(), type} };

    return ConversionProcessor::convert(e, args.front(), conv);
  }
  else if (type.isObjectType())
  {
    auto alloc = program::AllocateExpression::New(type.baseType());
    const std::vector<Function> & ctors = e->typeSystem()->getClass(type).constructors();
    OverloadResolution resol = OverloadResolution::New(e);

    if (!resol.process(ctors, args, alloc))
      throw CompilationFailure{ CompilerError::CouldNotFindValidConstructor };

    const Function ctor = resol.selectedOverload();
    const auto & inits = resol.initializations();
    for (std::size_t i(0); i < inits.size(); ++i)
    {
      const auto & init = inits.at(i);
      if (init.isNarrowing())
        throw CompilationFailure{ CompilerError::NarrowingConversionInBraceInitialization, errors::NarrowingConversion{args.at(i)->type(), ctor.parameter(i)} };
    }

    ValueConstructor::prepare(e, alloc, args, ctor.prototype(), inits);
    return program::ConstructorCall::New(ctor, alloc, std::move(args));
  }
  else
    throw NotImplemented{ "ValueConstructor::brace_construct() : type not implemented" };
}

std::shared_ptr<program::Expression> ValueConstructor::construct(Engine *e, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args)
{
  if (args.size() == 0)
    return construct(e, type, nullptr);

  if (!type.isObjectType() && args.size() != 1)
    throw CompilationFailure{ CompilerError::TooManyArgumentInInitialization };

  if ((type.isReference() || type.isRefRef()) && args.size() != 1)
    throw CompilationFailure{ CompilerError::TooManyArgumentInReferenceInitialization };

  if (type.isFundamentalType() || type.isEnumType() || type.isFunctionType())
  {
    Conversion conv = Conversion::compute(args.front(), type, e);

    if (conv == Conversion::NotConvertible())
      throw CompilationFailure{ CompilerError::CouldNotConvert, errors::ConversionFailure{args.front()->type(), type} };

    return ConversionProcessor::convert(e, args.front(), conv);
  }
  else if (type.isObjectType())
  {
    auto alloc = program::AllocateExpression::New(type.baseType());
    const std::vector<Function> & ctors = e->typeSystem()->getClass(type).constructors();
    OverloadResolution resol = OverloadResolution::New(e);

    if (!resol.process(ctors, args, alloc))
      throw CompilationFailure{ CompilerError::CouldNotFindValidConstructor };

    const Function ctor = resol.selectedOverload();
    const auto & inits = resol.initializations();
    ValueConstructor::prepare(e, alloc, args, ctor.prototype(), inits);
    return program::ConstructorCall::New(ctor, alloc, std::move(args));
  }
  else
    throw NotImplemented{ "ValueConstructor::construct() : type not implemented" };
}

std::shared_ptr<program::Expression> ValueConstructor::construct(ExpressionCompiler & ec, const Type & t, const std::shared_ptr<ast::ConstructorInitialization> & init)
{
  auto args = ec.generateExpressions(init->args);
  return construct(ec.engine(), t, std::move(args));
}

std::shared_ptr<program::Expression> ValueConstructor::construct(ExpressionCompiler & ec, const Type & t, const std::shared_ptr<ast::BraceInitialization> & init)
{
  auto args = ec.generateExpressions(init->args);
  return brace_construct(ec.engine(), t, std::move(args));
}


static void make_initializer_list(Class initalizer_list_type, program::InitializerList & ilist, const std::vector<Initialization> & inits)
{
  Type T = initalizer_list_type.arguments().front().type;
  for (size_t i(0); i < ilist.elements.size(); ++i)
    ilist.elements[i] = ValueConstructor::construct(initalizer_list_type.engine(), T, ilist.elements[i], inits.at(i));
  ilist.initializer_list_type = initalizer_list_type.id();
}

static std::shared_ptr<program::Expression> make_initializer_list(Class initalizer_list_type, std::vector<std::shared_ptr<program::Expression>> && elems, const std::vector<Initialization> & inits)
{
  const Type & T = initalizer_list_type.arguments().front().type;

  for (size_t i(0); i < elems.size(); ++i)
    elems[i] = ValueConstructor::construct(initalizer_list_type.engine(), T, elems.at(i), inits.at(i));

  auto ret = program::InitializerList::New(std::move(elems));
  ret->initializer_list_type = initalizer_list_type.id();
  return ret;
}

static std::shared_ptr<program::Expression> make_ctor_call(const Function & ctor, std::vector<std::shared_ptr<program::Expression>> && args, const std::vector<Initialization> & inits)
{
  for (size_t i(0); i < args.size(); ++i)
    args[i] = ValueConstructor::construct(ctor.engine(), ctor.parameter(i+1), args[i], inits.at(i));
  return program::ConstructorCall::New(ctor, std::move(args));
}

std::shared_ptr<program::Expression> ValueConstructor::construct(Engine *e, const Type & t, const std::shared_ptr<program::Expression> & arg, const Initialization & init)
{
  if (init.kind() == Initialization::DefaultInitialization)
    return construct(e, t, nullptr);


  if (init.kind() == Initialization::CopyInitialization
    || init.kind() == Initialization::ReferenceInitialization
    || (init.kind() == Initialization::DirectInitialization && !init.hasInitializations()))
  {
    return ConversionProcessor::convert(e, arg, init.conversion());
  }

  /// TODO: when available, handle aggregate initialization

  assert(init.kind() == Initialization::ListInitialization);
  assert(arg->is<program::InitializerList>());

  program::InitializerList & ilist = *std::static_pointer_cast<program::InitializerList>(arg);

  if (e->typeSystem()->isInitializerList(init.destType()))
  {
    make_initializer_list(e->typeSystem()->getClass(init.destType()), ilist, init.initializations());
    return arg;
  }
  else
  {
    const Function ctor = init.constructor();
    if (e->typeSystem()->isInitializerList(ctor.parameter(1)))
    {
      make_initializer_list(e->typeSystem()->getClass(ctor.parameter(1)), ilist, init.initializations());
      return program::ConstructorCall::New(ctor, { arg });
    }
    else
    {
      return make_ctor_call(ctor, std::move(ilist.elements), init.initializations());
    }
  }
}

std::shared_ptr<program::Expression> ValueConstructor::construct(Engine *e, const Type & t, std::vector<std::shared_ptr<program::Expression>> && args,  const Initialization & init)
{
  if (init.kind() == Initialization::DefaultInitialization)
    return construct(e, t, nullptr);


  if (init.kind() == Initialization::CopyInitialization
    || init.kind() == Initialization::ReferenceInitialization
    || (init.kind() == Initialization::DirectInitialization && !init.hasInitializations()))
  {
    return ConversionProcessor::convert(e, args.front(), init.conversion());
  }

  /// TODO: when available, handle aggregate initialization

  if (init.kind() == Initialization::DirectInitialization)
  {
    return make_ctor_call(init.constructor(), std::move(args), init.initializations());
  }

  assert(init.kind() == Initialization::ListInitialization);

  if (e->typeSystem()->isInitializerList(init.destType()))
  {
    return make_initializer_list(e->typeSystem()->getClass(init.destType()), std::move(args), init.initializations());
  }
  else
  {
    const Function ctor = init.constructor();
    if (e->typeSystem()->isInitializerList(ctor.parameter(0)))
    {
      auto ilist = make_initializer_list(e->typeSystem()->getClass(ctor.parameter(0)), std::move(args), init.initializations());
      return program::ConstructorCall::New(ctor, { ilist });
    }
    else
    {
      return make_ctor_call(ctor, std::move(args), init.initializations());
    }
  }
}

void ValueConstructor::prepare(Engine *e, std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<Initialization> & inits)
{
  for (size_t i(0); i < args.size(); ++i)
    args[i] = ValueConstructor::construct(e, proto.at(i), args.at(i), inits.at(i));
}

void ValueConstructor::prepare(Engine *e, std::shared_ptr<program::Expression> object, std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<Initialization> & inits)
{
  for (size_t i(0); i < args.size(); ++i)
    args[i] = ValueConstructor::construct(e, proto.at(i+1), args.at(i), inits.at(i+1));
}

} // namespace compiler

} // namespace script

