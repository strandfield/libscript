// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/conversionprocessor.h"

#include "script/engine.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/templateargument.h"
#include "script/typesystem.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/valueconstructor.h"

namespace script
{

namespace compiler
{

std::shared_ptr<program::Expression> ConversionProcessor::sconvert(Engine *e, const std::shared_ptr<program::Expression> & arg, const StandardConversion & conv)
{
  if (conv.isReferenceConversion())
    return arg;

  if (conv.isCopy())
  {
    return program::Copy::New(arg->type(), arg); /// TODO: remove this redundancy
  }
  else if (conv.isDerivedToBaseConversion())
  {
    const Class dest_class = e->typeSystem()->getClass(arg->type()).indirectBase(conv.derivedToBaseConversionDepth());
    return program::ConstructorCall::New(dest_class.copyConstructor(), { arg });
  }

  return program::FundamentalConversion::New(conv.destType(), arg);
}

std::shared_ptr<program::Expression> ConversionProcessor::convert(Engine *e, const std::shared_ptr<program::Expression> & arg, const Conversion & conv)
{
  if (conv == Conversion::NotConvertible())
    throw std::runtime_error{ "Cannot convert" };

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

