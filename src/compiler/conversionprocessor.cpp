// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/conversionprocessor.h"

#include "script/engine.h"
#include "script/cast.h"
#include "script/class.h"

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

std::shared_ptr<program::Expression> ConversionProcessor::listinit(Engine *e, const std::shared_ptr<program::Expression> & arg, const Type & type, const std::shared_ptr<ListInitializationSequence> & linit)
{
  assert(arg->is<program::InitializerList>());

  const program::InitializerList & ilist = *std::static_pointer_cast<program::InitializerList>(arg);

  if (linit->kind() == ListInitializationSequence::DefaultInitialization)
    return ValueConstructor::construct(e, linit->destType(), nullptr, diagnostic::pos_t{});
  
  if (linit->kind() != ListInitializationSequence::ConstructorListInitialization)
    throw NotImplementedError{ "Initializer list not implemented yet" };

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

script::Type ConversionProcessor::common_type(Engine *e, const std::shared_ptr<program::Expression> & a, const std::shared_ptr<program::Expression> & b)
{
  ConversionSequence conv_a = ConversionSequence::compute(a, b->type(), e);
  ConversionSequence conv_b = ConversionSequence::compute(b, a->type(), e);

  if (conv_a == ConversionSequence::NotConvertible() && conv_b == ConversionSequence::NotConvertible())
    return Type::Null;

  if(ConversionSequence::comp(conv_a, conv_b) < 0)
    return b->type();
  
  return a->type();
}

} // namespace compiler

} // namespace script

