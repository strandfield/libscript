// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILER_CONVERSION_PROCESSOR_H
#define LIBSCRIPT_COMPILER_CONVERSION_PROCESSOR_H

#include "script/conversions.h"

#include "script/program/expression.h"

namespace script
{

namespace compiler
{

class ConversionProcessor
{
public:

  static std::shared_ptr<program::Expression> sconvert(Engine *e, const std::shared_ptr<program::Expression> & arg, const Type & type, const StandardConversion & conv);
  static std::shared_ptr<program::Expression> listinit(const std::shared_ptr<program::Expression> & arg, const Type & type, const std::shared_ptr<ListInitializationSequence> & linit);
  static std::shared_ptr<program::Expression> convert(Engine *e, const std::shared_ptr<program::Expression> & arg, const Type & type, const ConversionSequence & conv);
  static void prepare(Engine *e, std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<ConversionSequence> & conversions);
  static script::Type common_type(Engine *e, const std::shared_ptr<program::Expression> & a, const std::shared_ptr<program::Expression> & b);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILER_CONVERSION_PROCESSOR_H
