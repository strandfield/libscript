// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/conversionprocessor.h"

#include "script/engine.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/templateargument.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/valueconstructor.h"

namespace script
{

namespace compiler
{

std::shared_ptr<program::Expression> ConversionProcessor::sconvert(Engine *e, const std::shared_ptr<program::Expression> & arg, const Type & type, const StandardConversion & conv)
{
  if (!conv.isCopyInitialization())
    return arg;

  assert(conv.isCopyInitialization());

  if (conv.isDerivedToBaseConversion() || type.isObjectType())
  {
    Class dest_class = e->getClass(type);
    return program::ConstructorCall::New(dest_class.copyConstructor(), { arg });
  }

  if (arg->type().baseType() == type.baseType())
    return program::Copy::New(type, arg);

  return program::FundamentalConversion::New(type, arg);
}

static void make_initializer_list(Class initalizer_list_type, program::InitializerList & ilist, const std::shared_ptr<ListInitializationSequence> & linit)
{
  Type T = initalizer_list_type.arguments().front().type;
  for (size_t i(0); i < ilist.elements.size(); ++i)
  {
    ilist.elements[i] = ConversionProcessor::convert(initalizer_list_type.engine(), ilist.elements[i], T, linit->conversions().at(i));
  }
  ilist.initializer_list_type = initalizer_list_type.id();
}

std::shared_ptr<program::Expression> ConversionProcessor::listinit(Engine *e, const std::shared_ptr<program::Expression> & arg, const Type & type, const std::shared_ptr<ListInitializationSequence> & linit)
{
  assert(arg->is<program::InitializerList>());

  program::InitializerList & ilist = *std::static_pointer_cast<program::InitializerList>(arg);

  if (linit->kind() == ListInitializationSequence::DefaultInitialization)
    return ValueConstructor::construct(e, linit->destType(), nullptr, diagnostic::pos_t{});

  if (linit->kind() == ListInitializationSequence::InitializerListCreation)
  {
    Class initalizer_list_type = e->getClass(linit->destType());
    make_initializer_list(initalizer_list_type, ilist, linit);
    return arg;
  }
  else if (linit->kind() == ListInitializationSequence::InitializerListInitialization)
  {
    Class initalizer_list_type = e->getClass(linit->constructor().parameter(0));
    make_initializer_list(initalizer_list_type, ilist, linit);
    return program::ConstructorCall::New(linit->constructor(), {arg});
  }
  
  if (linit->kind() != ListInitializationSequence::ConstructorListInitialization)
    throw NotImplementedError{ "This kind of list initialization is not implemented yet" };

  auto args = ilist.elements;
  prepare(e, args, linit->constructor().prototype(), linit->conversions());
  return program::ConstructorCall::New(linit->constructor(), std::move(args));
}

std::shared_ptr<program::Expression> ConversionProcessor::convert(Engine *e, const std::shared_ptr<program::Expression> & arg, const Type & type, const ConversionSequence & conv)
{
  if (conv.isListInitialization())
    return listinit(e, arg, type, conv.listInitialization);
  else if (!conv.isUserDefinedConversion())
    return sconvert(e, arg, type, conv.conv1);

  std::shared_ptr<program::Expression> ret = nullptr;

  if (conv.function.isCast())
  {
    auto cast = conv.function.toCast();
    ret = sconvert(e, arg, cast.sourceType(), conv.conv1);
    ret = program::FunctionCall::New(cast, { ret });
  }
  else
  {
    assert(conv.function.isConstructor());

    auto ctor = conv.function;
    ret = sconvert(e, arg, ctor.prototype().at(0), conv.conv1);
    ret = program::ConstructorCall::New(ctor, { ret });
  }

  return sconvert(e, ret, type, conv.conv3);
}

void ConversionProcessor::prepare(Engine *e, std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<ConversionSequence> & conversions)
{
  for (size_t i(0); i < args.size(); ++i)
  {
    args[i] = convert(e, args.at(i), proto.at(i), conversions.at(i));
  }
}

std::shared_ptr<program::Expression> ConversionProcessor::sconvert(Engine *e, const std::shared_ptr<program::Expression> & arg, const StandardConversion2 & conv)
{
  if (conv.isReferenceConversion())
    return arg;

  if (conv.isCopy())
  {
    return program::Copy::New(arg->type(), arg); /// TODO: remove this redundancy
  }
  else if (conv.isDerivedToBaseConversion())
  {
    const Class dest_class = e->getClass(arg->type()).indirectBase(conv.derivedToBaseConversionDepth());
    return program::ConstructorCall::New(dest_class.copyConstructor(), { arg });
  }

  return program::FundamentalConversion::New(conv.destType(), arg);
}

std::shared_ptr<program::Expression> ConversionProcessor::convert(Engine *e, const std::shared_ptr<program::Expression> & arg, const Conversion & conv)
{
  if (!conv.isUserDefinedConversion())
    return sconvert(e, arg, conv.firstStandardConversion());

  std::shared_ptr<program::Expression> ret = sconvert(e, arg, conv.firstStandardConversion());

  if (conv.userDefinedConversion().isCast())
  {
    ret = program::FunctionCall::New(conv.userDefinedConversion(), { ret });
  }
  else
  {
    assert(conv.userDefinedConversion().isConstructor());
    ret = program::ConstructorCall::New(conv.userDefinedConversion(), { ret });
  }

  return sconvert(e, ret, conv.secondStandardConversion());
}

script::Type ConversionProcessor::common_type(Engine *e, const std::shared_ptr<program::Expression> & a, const std::shared_ptr<program::Expression> & b)
{
  /// TODO: refactor

  Type dest_a2b = b->type();
  Conversion conv_a2b = Conversion::compute(a, dest_a2b, e);
  if (conv_a2b.isInvalid())
  {
    if (dest_a2b.isReference())
    {
      dest_a2b = dest_a2b.withoutRef();
      conv_a2b = Conversion::compute(a, dest_a2b, e);
    }

    if (conv_a2b.isInvalid())
      return Type::Null;
  }

  Type dest_b2a = a->type();
  Conversion conv_b2a = Conversion::compute(b, dest_b2a, e);
  if (conv_b2a.isInvalid())
  {
    if (dest_b2a.isReference())
    {
      dest_b2a = dest_b2a.withoutRef();
      conv_b2a = Conversion::compute(b, dest_b2a, e);
    }

    if (conv_b2a.isInvalid())
      return Type::Null;
  }

  if (conv_a2b == Conversion::NotConvertible() && conv_b2a == Conversion::NotConvertible())
    return Type::Null;

  if (Conversion::comp(conv_a2b, conv_b2a) < 0)
    return dest_a2b;
  
  return dest_b2a;
}

} // namespace compiler

} // namespace script

